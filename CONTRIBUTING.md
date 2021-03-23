Thanks for contributing to Cesium for Unreal!

Here are the guidelines that we use for all contributions to this project:

- [Submitting an issue](#submitting-an-issue),
- [Getting started contributing](#getting-started-contributing)
- [Opening a pull request](#opening-a-pull-request)

To ensure an inclusive community, contributors and users in the Cesium community should follow the [code of conduct](./CODE_OF_CONDUCT.md).

# Submitting an Issue

If you have a question, do not submit an issue; instead, search the [Cesium community forum](https://community.cesium.com/). The forum is very active and there are years of informative archives for the Cesium platform, often with answers from the core Cesium team. If you do not find an answer to your question, start a new thread and you'll likely get a quick response.

If you think you've found a bug in Cesium for Unreal, first search the [issues](https://github.com/CesiumGS/cesium-unreal/issues). If an issue already exists, please add a comment expressing your interest and any additional information. This helps us prioritize issues.

If a related issue does not exist, submit a new one. Please be concise and include as much of the following information as is relevant:

- The version of Cesium for Unreal. Did this work in a previous version?
- Your operating system and version, Unreal Engine version, and video card. Are they all up-to-date? Is the issue specific to one of them?
- If possible, an Unreal project (and data or IDs of Cesium ion assets that have been used) where the issue can be reproduced.
- Can the issue be reproduced in the [Cesium for Unreal demo](https://github.com/CesiumGS/cesium-unreal-demo)?
- A Screenshot, video or animated .gif if appropriate. Screenshots are particularly useful for exceptions and rendering artifacts.
- Information about whether the issue appeared in the Unreal Editor or in the play mode.
- If the issue is about a crash of the Unreal Editor, include the stack trace that is shown together with the error message.
- Depending on the type of the issue, it can be helpful to see the log files that have been created before encountering the unexpected behavior. The log files is stored as `Saved\Logs\cesiumunreal.log` in your project directory.
- A link to the thread if this was discussed on the [Cesium community forum](https://community.cesium.com/) or elsewhere.
- Ideas for how to fix or workaround the issue. Also mention if you are willing to help fix it. If so, the Cesium team can often provide guidance and the issue may get fixed more quickly with your help.

# Getting Started Contributing

Everyone is welcome to contribute to Cesium for Unreal!

In addition to contributing code, we appreciate many types of contributions:

- Being active on the [Cesium community forum](https://community.cesium.com/) by answering questions and providing input on Cesium's direction.
- Showcasing your Cesium for Unreal apps on [Cesium blog](https://cesium.com/blog/categories/userstories/). Contact us at hello@cesium.com.
- Writing tutorials, creating examples, and improving the reference documentation. See the issues labeled [doc](https://github.com/CesiumGS/cesium-unreal/labels/doc).
- Submitting issues as [described above](#submitting-an-issue).
- Triaging issues. Browse the [issues](https://github.com/CesiumGS/cesium-unreal/issues) and comment on issues that are no longer reproducible or on issues for which you have additional information.

For ideas for Cesium for Unreal code contributions, see:

- issues labeled [`good first issue`](https://github.com/CesiumGS/cesium-unreal/labels/good%20first%20issue) and
- issues labeled [`roadmap`](https://github.com/CesiumGS/cesium-unreal/labels/roadmap).

See the [build guide](https://github.com/CesiumGS/cesium-unreal#computer-developing-with-unreal-engine) for how to build and run Cesium for Unreal.

Always feel free to introduce yourself on the [Cesium community forum](https://community.cesium.com/) to brainstorm ideas and ask for guidance.

# Opening a Pull Request

We love pull requests. We strive to promptly review them, provide feedback, and merge. Interest in Cesium is at an all-time high so the core team is busy. Following the tips in this guide will help your pull request get merged quickly.

> If you plan to make a major change, please start a new thread on the [Cesium community forum](https://community.cesium.com/) first. Pull requests for small features and bug fixes can generally just be opened without discussion on the forum.

## Contributor License Agreement (CLA)

Before we can review a pull request, we require a signed Contributor License Agreement. There is a CLA for:

- [individuals](https://docs.google.com/forms/d/e/1FAIpQLScU-yvQdcdjCFHkNXwdNeEXx5Qhu45QXuWX_uF5qiLGFSEwlA/viewform) and
- [corporations](https://docs.google.com/forms/d/e/1FAIpQLSeYEaWlBl1tQEiegfHMuqnH9VxyfgXGyIw13C2sN7Fj3J3GVA/viewform).

This only needs to be completed once, and enables contributions to all of the projects under the [CesiumGS](https://github.com/CesiumGS) organization, including Cesium for Unreal. The CLA ensures you retain copyright to your contributions, and provides us the right to use, modify, and redistribute your contributions using the [Apache 2.0 License](LICENSE). If you have already signed a CLA for CesiumJS or other contributions to Cesium, you will not need to sign it again.

If you have any questions, feel free to reach out to [hello@cesium.com](mailto:hello@cesium)!

## Pull Request Guidelines

Our code is our lifeblood so maintaining Cesium's high code quality is important to us.

- For an overview of our workflow see [github pull request workflows](https://cesium.com/blog/2013/10/08/github-pull-request-workflows/).
- Pull request tips
  - After you open a pull request, the friendly [cesium-concierge](https://github.com/CesiumGS/cesium-concierge) bot will comment with a short automated review. At least one human will also review your pull request.
  - If your pull request fixes an existing issue, include a link to the issue in the description (like this: [#1](https://github.com/CesiumGS/cesium-unreal/issues/1)). Likewise, if your pull request fixes an issue reported on the Cesium forum, include a link to the thread.
  - If your pull request needs additional work, include a [task list](https://github.com/blog/1375%0A-task-lists-in-gfm-issues-pulls-comments).
  - Once you are done making new commits to address feedback, add a comment to the pull request such as `"this is ready"` since GitHub doesn't notify us about commits.
  - Follow the [Coding Guide](https://github.com/CesiumGS/cesium-native/blob/main/doc/style-guide.md).
  - Verify your is formatted, as described in the Coding Guide. 

## Code of Conduct

To ensure an inclusive community, contributors and users in the Cesium community should follow the [code of conduct](./CODE_OF_CONDUCT.md).
