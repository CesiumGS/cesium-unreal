/* eslint-disable strict */
/* eslint-disable n/no-process-exit */
const { postReleaseUpdateToTeamsChannel } = require('./teams-webhook.js'); 

async function main () {
  try {
    const releaseVersion = process.env.TAG_NAME;

    const response = await postReleaseUpdateToTeamsChannel(releaseVersion);

    console.log(`✅ Executed successfully for release version ${releaseVersion}.`);
    console.log(`✅ Microsoft Teams Workflow responded with status code ${response.status} - ${response.statusText}.`);

    process.exit(0);
  } catch (e) {
    console.error('🛑 Failed with error:\n', e);
    process.exit(1);
  }
}

main();