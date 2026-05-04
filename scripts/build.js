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

const args = process.argv.slice(2);
const target = args[0] || 'Editor';
const platform = args[1] || 'Win64';
const configuration = args[2] || 'Development';

const ENGINE_PATH_WIN = 'C:\\Program Files\\Epic Games\\UE_5.7\\Engine';
const ENGINE_PATH = winToWslPath(ENGINE_PATH_WIN);
const UBT_PATH = path.join(ENGINE_PATH, 'Build', 'BatchFiles', 'Build.bat');
const PROJECT_PATH = path.resolve(__dirname, '..', 'echelon_05.uproject');
const PROJECT_NAME = 'echelon_05';

if (!fs.existsSync(ENGINE_PATH)) {
    console.error(`Error: Unreal Engine not found at ${ENGINE_PATH}`);
    console.error('Please update the ENGINE_PATH in scripts/build.js to match your installation.');
    process.exit(1);
}

const UBT_PATH_WIN = path.join(ENGINE_PATH_WIN, 'Build', 'BatchFiles', 'Build.bat');
const PROJECT_PATH_WIN = wslToWinPath(PROJECT_PATH);

let buildCommand;
if (isWSL) {
    buildCommand = `powershell.exe -Command "& '${UBT_PATH_WIN}' ${PROJECT_NAME}${target} ${platform} ${configuration} '${PROJECT_PATH_WIN}'"`;
} else {
    buildCommand = `"${UBT_PATH}" ${PROJECT_NAME}${target} ${platform} ${configuration} "${PROJECT_PATH}"`;
}

console.log(`Building ${PROJECT_NAME}${target} for ${platform} (${configuration})...`);
console.log(`Command: ${buildCommand}\n`);

try {
    execSync(buildCommand, {
        stdio: 'inherit',
        cwd: path.resolve(__dirname, '..')
    });
    console.log(`\n✓ Build completed successfully!`);
} catch (error) {
    console.error(`\n✗ Build failed!`);
    process.exit(1);
}
