# Mini Lightroom App (C++/WebAssembly)

This project is a minimal implementation of a Lightroom-style image editor, compiled to WebAssembly using Emscripten.

## Build

- clone the repo 
- cd wasm_dng_sdk
- mkdir build
- cd build
-  `cmake .. -DCMAKE_BUILD_TYPE=Release` (or a different build type)
- run `emmake make`


## Prerequisites

- [Emscripten](https://emscripten.org/docs/getting_started/downloads.html) installed and configured
- A modern web browser

## Running the App

1. Start the local server using `emrun`:
   ```bash
   emrun --no_browser --port <PORT> hello.html


Note: emrun is part of the Emscripten SDK. Ensure it is available in your environment.
(`emrun` is in path `wasm_dng_sdk\emsdk\upstream\emscripten\emrun` )


2. Open a web browser and navigate to::
   ```bash
   http://localhost:<PORT>/hello.html


## Test Helpers
The `test_helpers/` directory includes:

A sample image from an EOS R6 camera

A matching .dcp (DNG Camera Profile) file for testing color profiles

