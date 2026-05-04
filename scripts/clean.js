const { execSync } = require('child_process');
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
const CLEAN_BATCH = path.join(ENGINE_PATH, 'Build', 'BatchFiles', 'Clean.bat');
const CLEAN_BATCH_WIN = path.join(ENGINE_PATH_WIN, 'Build', 'BatchFiles', 'Clean.bat');
const PROJECT_PATH = path.resolve(__dirname, '..', 'echelon_05.uproject');
const PROJECT_PATH_WIN = wslToWinPath(PROJECT_PATH);
const PROJECT_NAME = 'echelon_05';

if (!fs.existsSync(ENGINE_PATH)) {
    console.error(`Error: Unreal Engine not found at ${ENGINE_PATH}`);
    console.error('Please update the ENGINE_PATH in scripts/clean.js to match your installation.');
    process.exit(1);
}

console.log('Cleaning build artifacts...\n');

let cleanCommand;
if (isWSL) {
    cleanCommand = `powershell.exe -Command "& '${CLEAN_BATCH_WIN}' ${PROJECT_NAME}Editor Win64 Development '${PROJECT_PATH_WIN}'"`;
} else {
    cleanCommand = `"${CLEAN_BATCH}" ${PROJECT_NAME}Editor Win64 Development "${PROJECT_PATH}"`;
}

try {
    execSync(cleanCommand, {
        stdio: 'inherit',
        cwd: path.resolve(__dirname, '..')
    });
    console.log(`\n✓ Clean completed successfully!`);
} catch (error) {
    console.error(`\n✗ Clean failed!`);
    process.exit(1);
}
