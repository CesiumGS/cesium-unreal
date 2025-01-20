# Contribution Guide {#contributing-unreal}

Thanks for contributing to Cesium for Unreal!

<!--!
[TOC]
-->

Here are the guidelines that we use for all contributions to this project:

- [Submitting an issue](#submitting-an-issue),
- [Getting started contributing](#getting-started-contributing)
- [Opening a pull request](#opening-a-pull-request)

To ensure an inclusive community, contributors and users in the Cesium community should follow the [code of conduct](./CODE_OF_CONDUCT.md).

# Submitting an Issue {#submitting-an-issue}

If you have a question, **do not submit an issue**. Instead, search the [Cesium community forum](https://community.cesium.com/) for your question. The forum is very active and there are years of informative archives for the Cesium platform, often with answers from the core Cesium team. If you do not find an answer to your question, start a new thread and you'll likely get a quick response.

If you think you've found a bug in Cesium for Unreal, first search the existing [issues](https://github.com/CesiumGS/cesium-unreal/issues). If an issue already exists, please add a **comment** expressing your interest and any additional information. This helps us stay organized and prioritize issues.

If a related issue does not exist, then you can submit a new one. Please include as much of the following information as is relevant:

- What version of Cesium for Unreal were you using? Do other versions have the same issue?
- What version of Unreal Engine were you using?
- What is your operating system and version, and your video card? Are they up-to-date? Is the issue specific to one of them?
- Can the issue be reproduced in the [Cesium for Unreal Samples](https://github.com/CesiumGS/cesium-unreal-samples)?
- Does the issue occur in the Unreal Editor? In Play Mode? Both?
- Are there any error messages in the console? Any stack traces or logs? Please include them if so. Logs are stored as `Saved\Logs\cesiumunreal.log` in your project directory.
- Share a screenshot, video or animated `.gif` if appropriate. Screenshots are particularly useful for exceptions and rendering artifacts.
- Link to threads on the [Cesium community forum](https://community.cesium.com/) if this was discussed on there.
- Include step-by-step instructions for us to reproduce the issue from scratch.
- Any ideas for how to fix or workaround the issue. Also mention if you are willing to help fix it. If so, the Cesium team can often provide guidance and the issue may get fixed more quickly with your help.

**Note**: It is difficult for us to debug everyone's individual projects. We can triage issues faster when we receive steps to reproduce the issue from scratch—either from [Cesium for Unreal Samples](https://github.com/CesiumGS/cesium-unreal-samples) or from a blank project. We will only request your Unreal project and/or data as a last resort.

# Getting Started Contributing {#getting-started-contributing}

Everyone is welcome to contribute to Cesium for Unreal!

In addition to contributing code, we appreciate many types of contributions:

- Being active on the [Cesium community forum](https://community.cesium.com/) by answering questions and providing input on Cesium's direction.
- Showcasing your Cesium for Unreal apps on [Cesium blog](https://cesium.com/blog/categories/userstories/). Contact us at hello@cesium.com.
- Writing tutorials, creating examples, and improving the reference documentation. See the issues labeled [documentation](https://github.com/CesiumGS/cesium-unreal/labels/documentation).
- Submitting issues as [described above](#submitting-an-issue).
- Triaging issues. Browse the [issues](https://github.com/CesiumGS/cesium-unreal/issues) and comment on issues that are no longer reproducible or on issues for which you have additional information.

For ideas for Cesium for Unreal code contributions, see:

- issues labeled [`good first issue`](https://github.com/CesiumGS/cesium-unreal/labels/good%20first%20issue),
- issues labeled [`low hanging fruit`](https://github.com/CesiumGS/cesium-unreal/labels/low%20hanging%20fruit), and
- issues labeled [`enhancement`](https://github.com/CesiumGS/cesium-unreal/labels/enhancement).

See [Developer Setup](https://cesium.com/learn/cesium-unreal/ref-doc/developer-setup-unreal.html) for how to build and run Cesium for Unreal.

Always feel free to introduce yourself on the [Cesium community forum](https://community.cesium.com/) to brainstorm ideas and ask for guidance.

# Opening a Pull Request {#opening-a-pull-request}

We love pull requests. We strive to promptly review them, provide feedback, and merge. Interest in Cesium is at an all-time high so the core team is busy. Following the tips in this guide will help your pull request get merged quickly.

> If you plan to make a major change, please start a new thread on the [Cesium community forum](https://community.cesium.com/) first. Pull requests for small features and bug fixes can generally just be opened without discussion on the forum.

## Contributor License Agreement (CLA)

Before we can review a pull request, we require a signed Contributor License Agreement. The CLA forms can be found in our `community` repo [here](https://github.com/CesiumGS/community/tree/main/CLAs).

## Pull Request Guidelines

Our code is our lifeblood so maintaining Cesium's high code quality is important to us.

- For an overview of our workflow see [github pull request workflows](https://cesium.com/blog/2013/10/08/github-pull-request-workflows/).
- Pull request tips
  - After you open a pull request, one or more Cesium teammates will review your pull request.
  - If your pull request fixes an existing issue, include a link to the issue in the description (like this: [#1](https://github.com/CesiumGS/cesium-unreal/issues/1)). Likewise, if your pull request fixes an issue reported on the Cesium forum, include a link to the thread.
  - If your pull request needs additional work, include a [task list](https://github.com/blog/1375%0A-task-lists-in-gfm-issues-pulls-comments).
  - Once you are done making new commits to address feedback, add a comment to the pull request such as `"this is ready"` so we know to take another look.
  - Verify that your code conforms to our [Style Guide](https://github.com/CesiumGS/cesium-native/blob/main/doc/style-guide.md).

## Code of Conduct

To ensure an inclusive community, contributors and users in the Cesium community should follow the [code of conduct](./CODE_OF_CONDUCT.md).
