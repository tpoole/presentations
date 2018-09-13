# Presentations

This repository contains slides and supporting material for talks I've given.

###### Index:
  - [Why and How to Roll Your Own std::function Implementation](#why-and-how-to-roll-your-own-stdfunction-implementation)
  - [Develop large scale projects with JUCE](#develop-large-scale-projects-with-juce)

### Why and How to Roll Your Own std::function Implementation
###### CppCon (and ACCU Bristol) - September 2018 : [Slides (coming soon)](link_coming_soon) - [Video (coming soon)](link_coming_soon) - [Resources](https://github.com/tpoole/presentations/tree/master/CppCon%202018%20-%20Why%20and%20How%20to%20Roll%20Your%20Own%20std::function%20Implementation)

In recent years the increased usage of std::function has transformed the way many C++ programs are written. However, if your application is processing realtime data, or doing some other performance critical task, then the possibility of std::function allocating some memory from the heap may not be tolerable. It's also possible that the systems you are targeting simply lack a std::function implementation, preventing its adoption in applications for legacy operating systems, toolchains for embedded devices, and inside open source library code. Rolling your own implementation of std::function can provide a solution to both of these concerns simultaneously, allowing you to modernize your code and provide guarantees about the runtime performance of manipulating function objects. 

This presentation outlines why and how a std::function replacement was added to the JUCE open source, cross platform, software development framework and discusses some differences between our implementation and others. We will also cover how we can move beyond the standard interface by extending the small buffer optimization to make manipulating callable objects more suitable for performance critical and realtime contexts, finishing with some examples of how this applies to processing live audio data.

### Develop large scale projects with JUCE
###### ADC - November 2017 : *Paid workshop content on request*

Led by JUCE's founder Jules Storer and senior software engineer Tom Poole, this workshop will be particularly helpful for companies whose products and teams are scaling and who look to manage the complexity of large projects. In particular, attendees can expect to improve their knowledge in the following areas:

  - How to cope with large codebases and large builds
  - How to cope with multiple platforms and deployment targets
  - How to cope with large teams collaborating on a project

In addition to tips from JUCE experts, the session will aim at discussing and addressing attendees' issues as encountered in the field. 
