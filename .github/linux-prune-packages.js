const childProcess = require('child_process');

const packagesToKeep = [
  "apt",
  "binutils",
  "coreutils",
  "core22",
  "pc-kernel",
  "pc",
  "snapd"
];

function runCommand(command) {
  return childProcess.execSync(command, { encoding: 'utf-8' });
}

const packagesList = runCommand("dpkg-query -Wf '${Package}\n'").trim().split("\n");
const packagesObj = {};
packagesList.forEach(pkg => {
  packagesObj[pkg] = 'remove';
});

function findPackageDependencies(packageName) {
  const regex = /(Depends: |Recommends: )([^\s]+)/g;
  const result = runCommand(`apt depends ${packageName}`);
  const matches = [...result.matchAll(regex)];
  matches.forEach(match => {
    const name = match[2].replace(/[\<\>]/g, '');
    if(packagesObj[name] === 'remove') {
      packagesObj[name] = 'unchecked';
    }
  });
}

// Make sure we're not trying to keep packages that don't exist.
packagesToKeep.filter(pkg => packagesObj.hasOwnProperty(pkg)).forEach(pkg => {
  packagesObj[pkg] = 'unchecked';
});

function runDependencyCheckIteration() {
  const uncheckedPackages = Object.keys(packagesObj).filter(pkg => packagesObj[pkg] === 'unchecked');
  uncheckedPackages.forEach(pkg => {
    findPackageDependencies(pkg);
    packagesObj[pkg] = 'keep';
  });
}

while (Object.values(packagesObj).includes('unchecked')) {
  runDependencyCheckIteration();
}

console.log(Object.keys(packagesObj).filter(pkg => packagesObj[pkg] === 'remove').join(' '));