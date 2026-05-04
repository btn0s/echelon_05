const { spawn, exec } = require('child_process');
const path = require('path');
const fs = require('fs');

const isWSL = fs.existsSync('/proc/version') &&
    fs.readFileSync('/proc/version', 'utf8').toLowerCase().includes('microsoft');

function winToWslPath(winPath) {
    if (!isWSL) return winPath;
    return winPath
        .replace(/^([A-Za-z]):\\/, (_, drive) => `/mnt/${drive.toLowerCase()}/`)
        .replace(/\\/g, '/');
}

function wslToWinPath(wslPath) {
    if (!isWSL) return wslPath;
    return wslPath
        .replace(/^\/mnt\/([a-z])\//, (_, drive) => `${drive.toUpperCase()}:\\`)
        .replace(/\//g, '\\');
}

const ENGINE_PATH_WIN = 'C:\\Program Files\\Epic Games\\UE_5.7\\Engine';
const ENGINE_PATH = winToWslPath(ENGINE_PATH_WIN);
const EDITOR_PATH = path.join(ENGINE_PATH, 'Binaries', 'Win64', 'UnrealEditor.exe');
const EDITOR_PATH_WIN = path.join(ENGINE_PATH_WIN, 'Binaries', 'Win64', 'UnrealEditor.exe');
const PROJECT_PATH = path.resolve(__dirname, '..', 'echelon_05.uproject');
const PROJECT_PATH_WIN = wslToWinPath(PROJECT_PATH);
const LOG_DIR = path.resolve(__dirname, '..', 'Saved', 'Logs');
const LOG_FILE = path.join(LOG_DIR, 'echelon_05.log');

if (!fs.existsSync(ENGINE_PATH)) {
    console.error(`Error: Unreal Engine not found at ${ENGINE_PATH}`);
    console.error('Please update the ENGINE_PATH in scripts/launch.js to match your installation.');
    process.exit(1);
}

if (!fs.existsSync(EDITOR_PATH)) {
    console.error(`Error: UnrealEditor.exe not found at ${EDITOR_PATH}`);
    console.error('The editor may need to be built first. Run: npm run build');
    process.exit(1);
}

if (!fs.existsSync(PROJECT_PATH)) {
    console.error(`Error: Project file not found at ${PROJECT_PATH}`);
    process.exit(1);
}

if (!fs.existsSync(LOG_DIR)) {
    fs.mkdirSync(LOG_DIR, { recursive: true });
}

console.log(`Launching Unreal Editor for ${path.basename(PROJECT_PATH)}...`);
console.log(`Editor: ${EDITOR_PATH}`);
console.log(`Project: ${PROJECT_PATH}`);
console.log(`\n=== Unreal Engine Logs (tailing ${path.basename(LOG_FILE)}) ===\n`);

let editorProcess;
if (isWSL) {
    editorProcess = spawn('powershell.exe', ['-Command', `& '${EDITOR_PATH_WIN}' '${PROJECT_PATH_WIN}'`], {
        detached: true,
        stdio: 'ignore',
        cwd: path.resolve(__dirname, '..')
    });
} else {
    editorProcess = spawn(EDITOR_PATH, [PROJECT_PATH], {
        detached: true,
        stdio: 'ignore',
        cwd: path.resolve(__dirname, '..')
    });
}

editorProcess.unref();

let logWatcher = null;
let lastPosition = 0;
let lineCount = 0;
const MAX_LINES_IN_FILE = 100;
const TRIM_INTERVAL_LINES = 50;

function waitForLogFile(maxAttempts = 30) {
    if (fs.existsSync(LOG_FILE)) {
        startTailingLog();
        return;
    }

    if (maxAttempts > 0) {
        setTimeout(() => waitForLogFile(maxAttempts - 1), 500);
    } else {
        console.log('Warning: Log file not found. Editor may have failed to start.');
        console.log('Check if the editor window opened.');
    }
}

function trimLogFile() {
    try {
        if (!fs.existsSync(LOG_FILE)) return;

        const content = fs.readFileSync(LOG_FILE, 'utf8');
        const lines = content.split('\n');

        if (lines.length > MAX_LINES_IN_FILE) {
            const trimmedLines = lines.slice(-MAX_LINES_IN_FILE);
            const trimmedContent = trimmedLines.join('\n');

            fs.writeFileSync(LOG_FILE, trimmedContent, 'utf8');

            lastPosition = Buffer.byteLength(trimmedContent, 'utf8');
            lineCount = MAX_LINES_IN_FILE;
        }
    } catch (err) {
        // File might be locked
    }
}

function startTailingLog() {
    if (fs.existsSync(LOG_FILE)) {
        try {
            const content = fs.readFileSync(LOG_FILE, 'utf8');
            lineCount = content.split('\n').length;
            lastPosition = content.length;
        } catch (err) {
            // File might be locked
        }
    }

    logWatcher = fs.watchFile(LOG_FILE, { interval: 250 }, (curr, prev) => {
        if (curr.size > lastPosition) {
            try {
                const stream = fs.createReadStream(LOG_FILE, {
                    start: lastPosition,
                    end: curr.size,
                    encoding: 'utf8'
                });

                let newChunk = '';
                stream.on('data', (chunk) => {
                    newChunk += chunk;
                    process.stdout.write(chunk);
                });

                stream.on('end', () => {
                    const newLines = newChunk.split('\n').length - 1;
                    lineCount += newLines;
                    lastPosition = curr.size;

                    if (lineCount >= TRIM_INTERVAL_LINES) {
                        trimLogFile();
                    }
                });

                stream.on('error', () => {});
            } catch (err) {
                // File might be locked
            }
        }
    });

    setInterval(() => {
        if (fs.existsSync(LOG_FILE)) {
            trimLogFile();
        }
    }, 30000);
}

waitForLogFile();

let isShuttingDown = false;
process.on('SIGINT', () => {
    if (isShuttingDown) return;
    isShuttingDown = true;

    console.log('\n\n=== Shutting down editor... ===');

    if (logWatcher) {
        fs.unwatchFile(LOG_FILE);
    }

    try {
        const killCmd = isWSL
            ? `powershell.exe -Command "Stop-Process -Name UnrealEditor -Force -ErrorAction SilentlyContinue"`
            : `taskkill /F /IM UnrealEditor.exe`;
        exec(killCmd, (error) => {
            if (error) {
                console.log('Editor process may have already exited.');
            }
            process.exit(0);
        });
    } catch (err) {
        console.log('Could not terminate editor process. Please close it manually.');
        process.exit(0);
    }
});

process.on('SIGTERM', () => {
    if (logWatcher) {
        fs.unwatchFile(LOG_FILE);
    }
    process.exit(0);
});
