const childProcess = require('child_process');

const packagesToKeep = [
  "apt",
  "binutils",
  "coreutils",
  "core22",
  "pc-kernel",
  "pc",
  "snapd",
  "sudo",
  "init",
  "shim-signed",
  "unzip",
  "git"
];

function runCommand(command) {
  return childProcess.execSync(command, { encoding: 'utf-8' });
}

const packagesList = runCommand("dpkg-query -Wf '${Package} ${Priority} ${Essential}\n'").trim().split("\n");
const packagesObj = {};
packagesList.forEach(pkgInfo => {
  const parts = pkgInfo.split(' ');
  const pkg = parts[0];
  const required = parts[1] == 'required';
  const essential = parts[2] == 'yes';
  if(required || essential) {
    packagesObj[pkg] = 'unchecked';
  }
  else {
    packagesObj[pkg] = 'remove';
  }
});

function findPackageDependencies(packages) {
  const regex = /(Depends: |Recommends: )([^\s]+)/g;
  const result = runCommand(`apt depends ${packages.join(' ')}`);
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
  findPackageDependencies(uncheckedPackages);
  uncheckedPackages.forEach(pkg => {
    packagesObj[pkg] = 'keep';
  });
}

while (Object.values(packagesObj).includes('unchecked')) {
  runDependencyCheckIteration();
}

console.log(Object.keys(packagesObj).filter(pkg => packagesObj[pkg] === 'remove').join(' '));