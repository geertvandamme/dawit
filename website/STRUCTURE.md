# DawIt website structure

The site should answer the questions a visitor has in order: what is this, why would I use it, how does it help me learn, what can I do with it, and how do I begin?

The seven-page site is maintained in `website/`. On 29 October 2026 the user requested that this redesign replace the former site in that folder. This describes the current local structure, not a deployment.

## Primary navigation

The logo returns to Home. The navigation contains **Why DawIt**, **Learn**, **Features**, **For you**, **How it’s built** and **Get started**. How it’s built is also in the footer and keeps the existing `about.html` URL. It explains human decisions, AI-assisted development, the app’s building blocks, local models and sound checks before the complete expandable credits.

Every footer also links to the public [GitHub repository](https://github.com/geertvandamme/dawit). Get started has a prominent repository link beside source-build availability; How it’s built links to it from the introduction. Source publication and installer availability are described separately.

| Page | The visitor’s question | Content |
| --- | --- | --- |
| Home | What is DawIt? | A music sketchpad that explains itself; the three kinds of learning support; who it suits; routes into features; moving an idea to a full DAW. |
| Why DawIt | How does this differ from a full DAW? | A plain definition of DAW; a comparison of priorities; a concrete compressor example; how the two tools work together; early-access expectations. |
| Learn | What does learning inside the app look like? | Theory guides, playable examples, real-time explanations, live pictures, “Your song” topics, an eight-step beginner path and a searchable subject library. |
| Features | Can it do the thing I need? | All 53 original feature sections, regrouped into six tasks with search, subject filters, expandable details and direct links. |
| For you | Is this useful for someone like me? | All eight original roles, their first action, relevant tools and screenshots. |
| Get started | How do I try it? | Accurate availability, source-build instructions, a first-session path, optional local models and practical answers. |
| How it’s built | Who made it, and what does it depend on? | Human decisions, AI coding tools, libraries, models, credits, licences and the site’s actual implementation. |

## The message to make clear

DawIt has real tracks, MIDI and audio recording, instruments, effects, buses and sends. Its distinguishing purpose is learning and trying ideas with explanations close to the work.

The three things to demonstrate are:

1. **Theory guides:** plain-language topics, pictures and examples you can hear. Many examine your own song, track or clip.
2. **Help in the moment:** controls explain what they do and what the current settings mean. Starting points give reasons. Chord functions, meters and contextual advice help with the next choice.
3. **Live feedback:** pictures and measurements follow the song or settings, making a change something you can see and hear.

A full DAW supports developing and finishing a production. DawIt prepares ideas and understanding that carry into that workflow. The comparison acknowledges that full DAWs have learning resources too; it does not claim that they lack explanations or share an identical feature set.

## Feature organisation

- **Write & compose:** keys, chords, structure, lyrics, rhythm, AI and MIDI editing.
- **Play, sing & record:** chord diagrams, rehearsal views, chord sheets, singing, MIDI and audio recording, computer keys and tuning.
- **Explore a recording:** stems, karaoke, audio and drum hits to MIDI, tempo, changing tempo, chords, sections and lyrics.
- **Mix & understand:** sound and MIDI foundations, effects chains, levels, buses, returns, inspector, signal flow and the nine built-in effects.
- **Make a sound:** subtractive, additive, FM, drum synthesis and physical modelling.
- **Work & export:** the screen, Theory Guides window, plugin builds and MIDI export.

The feature page retains the existing feature anchor names. Links from guides and role descriptions therefore go straight to the right expandable section. Opening a direct link reveals that feature even if a previous search hid it.

## Why this is easier to navigate

The former overview combined introductory positioning, a large guide picker, feature lists and role links. Its feature page carried a long succession of detailed sections. The current site gives each page a single purpose and lets people choose the depth they need.

Learning is a primary destination because it is DawIt’s central difference. Features are browsable by the task, rather than asking a beginner to recognise a technical term. Roles offer a second route into the same features. How it’s built offers a visible route for visitors curious about the development process; full technical credits remain expandable within that page.

## Visual direction

An open music notebook: warm paper, dark green ink, muted chord colours and a restrained serif accent. The original book-and-levels logo remains. Real app images establish what the product actually does. Light and dark themes, phone layouts and keyboard focus states are included. System fonts avoid external font requests.

Every app screenshot is copied unchanged from the existing site. Full-size links make the screenshots easier to inspect on a phone. No app screenshots were captured, generated or altered.
