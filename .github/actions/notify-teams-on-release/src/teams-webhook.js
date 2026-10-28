/* eslint-disable strict */
const axios = require("axios");
const fs = require("fs");

const TEAMS_WEBHOOK_URL = process.env.TEAMS_WEBHOOK_URL;
const DEFAULT_PAYLOAD_FILE_PATH = "default-payload.json";

const COLLEAUGES_TO_TAG = [
  { name: "Ben Jewett", id: "ben.jewett@bentley.com" },
  { name: "Cory Barr", id: "cory.barr@bentley.com" },
  { name: "Darcy Van der Drift", id: "Darcy.VanDerDrift@bentley.com" },
  { name: "Jake Steinerman", id: "Jake.Steinerman@bentley.com" },
  { name: "Travis Crowell", id: "Travis.Crowell@bentley.com" }
];

function buildPayloadEntityForMentions(colleaguesToTag) {
  return colleaguesToTag.map(eachColleague => ({
    type: "mention",
    text: `<at>${eachColleague.name}</at>`,
    mentioned: { id: eachColleague.id, name: eachColleague.name }
  }));
}

function buildTeamsMessageFromReleaseVersion(releaseVersion, colleaguesToTag) {
  const body = [
    {
      type: "TextBlock",
      text: `Cesium for Unreal ${releaseVersion} is [now available](https://github.com/CesiumGS/cesium-unreal/releases) 🎉`,
      size: "Medium",
      wrap: true,
    }
  ];

  if (colleaguesToTag?.length) {  
    const namesList = colleaguesToTag.map(p => `<at>${p.name}</at>`).join(" ");
    body.push({
      type: "TextBlock",
      size: "Small",
      text: `cc ${namesList}`,
      wrap: true
    });
  }  

  return body;
}

async function postReleaseUpdateToTeamsChannel(releaseVersion) {
  const defaultTeamsMessagePayload = JSON.parse(fs.readFileSync(DEFAULT_PAYLOAD_FILE_PATH, "utf8"));

  const cardToDisplayAsMessage = defaultTeamsMessagePayload.attachments[0].content;

  // Adds our custom message to the default card that will be displayed in the Teams channel
  cardToDisplayAsMessage.body.push(...buildTeamsMessageFromReleaseVersion(releaseVersion, COLLEAUGES_TO_TAG));

  // Adds a map between colleague names and emails to the payload, so anyone tagged get notified in Teams
  cardToDisplayAsMessage.msteams.entities.push(...buildPayloadEntityForMentions(COLLEAUGES_TO_TAG));

  const response = await axios.post(TEAMS_WEBHOOK_URL, defaultTeamsMessagePayload, {
    headers: { "Content-Type": "application/json" }
  });

  return response;
};

module.exports = { postReleaseUpdateToTeamsChannel };