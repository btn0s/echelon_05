const path = require('path');
const fs = require('fs');

const LOG_DIR = path.resolve(__dirname, '..', 'Saved', 'Logs');
const PROJECT_NAME = 'echelon_05';

const KEEP_LOGS = parseInt(process.argv[2]) || 5;

if (!fs.existsSync(LOG_DIR)) {
    console.log('Log directory does not exist. Nothing to clean.');
    process.exit(0);
}

console.log(`Cleaning old log files in ${LOG_DIR}...`);
console.log(`Keeping the ${KEEP_LOGS} most recent backup logs.\n`);

try {
    const files = fs.readdirSync(LOG_DIR);

    const backupLogs = files
        .filter(file => file.startsWith(`${PROJECT_NAME}-backup-`) && file.endsWith('.log'))
        .map(file => ({
            name: file,
            path: path.join(LOG_DIR, file),
            stats: fs.statSync(path.join(LOG_DIR, file))
        }))
        .sort((a, b) => b.stats.mtimeMs - a.stats.mtimeMs);

    if (backupLogs.length === 0) {
        console.log('No backup log files found.');
        process.exit(0);
    }

    const toDelete = backupLogs.slice(KEEP_LOGS);
    const toKeep = backupLogs.slice(0, KEEP_LOGS);

    if (toDelete.length === 0) {
        console.log(`All ${backupLogs.length} backup log(s) are within the keep limit. Nothing to delete.`);
        process.exit(0);
    }

    console.log(`Keeping ${toKeep.length} most recent backup log(s):`);
    toKeep.forEach((file, index) => {
        const sizeMB = (file.stats.size / (1024 * 1024)).toFixed(2);
        console.log(`  ${index + 1}. ${file.name} (${sizeMB} MB)`);
    });

    console.log(`\nDeleting ${toDelete.length} old backup log(s):`);
    let totalFreed = 0;
    toDelete.forEach((file) => {
        const sizeMB = (file.stats.size / (1024 * 1024)).toFixed(2);
        totalFreed += file.stats.size;
        try {
            fs.unlinkSync(file.path);
            console.log(`  ✓ Deleted ${file.name} (${sizeMB} MB)`);
        } catch (err) {
            console.error(`  ✗ Failed to delete ${file.name}: ${err.message}`);
        }
    });

    const totalFreedMB = (totalFreed / (1024 * 1024)).toFixed(2);
    console.log(`\n✓ Cleanup complete! Freed ${totalFreedMB} MB.`);
} catch (error) {
    console.error(`✗ Failed to clean logs: ${error.message}`);
    process.exit(1);
}
