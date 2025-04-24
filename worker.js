// onmessage = function(e) {
//   const f = e.data[0];
//
//   FS.mkdir('/work');
//   console.log("hello!!!!!!!", WORKERFS);
//   // FS.mount(OPFS, { files: [f] }, '/work');
//
//   FS.mount(WORKERFS, { files: [f] }, '/work');
//   //
//   console.log(Module.read_file('/work/' + f.name));
// }
//
// self.importScripts('./build/dng_example.js');

// import initModule from './build/dng_example.js'; // assumes hello.js was built with -s MODULARIZE=1 -s EXPORT_ES6=1
//
// let Module;
//
// initModule().then((mod) => {
//   Module = mod;
//
//   // Set up FS after module is ready
//   self.onmessage = function (e) {
//     const f = e.data[0];
//
//     Module.FS.mkdir('/work');
//     Module.FS.mount(Module.WORKERFS, { files: [f] }, '/work');
//
//     const contents = Module.read_file('/work/' + f.name);
//     console.log('Read file from worker:', contents);
//   };
// });
// import initModule from './build/dng_example.js';
//
// let Module;
//
// initModule().then((mod) => {
//
//
//   Module.onRuntimeInitialized = () => {
//     self.postMessage({ log: "✅ Worker is ready" });
//
//     console.log('✅ WASM Module ready inside worker');
//     self.onmessage = function (e) {
//       console.log("✅ Main thread received from worker:", e.data);
//
//       const file = e.data[0];
//       console.log('📨 Received file in worker:', file);
//
//       Module.FS.mkdir('/work');
//       Module.FS.mount(Module.WORKERFS, { files: [file] }, '/work');
//
//       const contents = Module.read_file('/work/' + file.name);
//       console.log('📤 Read file contents:', contents);
//
//       // 🔥 Send the content back to main thread!
//       self.postMessage({ result: contents });
//     };
//
//   };
//   Module = mod;
//   console.log("📦 Module initialized");
// });
import initModule from './build/dng_example.js';

let Module;

initModule({
  onRuntimeInitialized() {
    console.log("✅ WASM Module ready inside worker");
    self.postMessage({ log: "✅ Worker is ready" });

    self.onmessage = function (e) {
      const file = e.data[0];
      console.log("📨 Got file in worker", file);

      try {
        Module.FS.mkdir('/work');
      } catch (err) {
        console.warn("📁 mkdir failed (maybe already exists)", err);
      }

      try {
        Module.FS.mount(Module.WORKERFS, { files: [file] }, '/work');
        const content = Module.read_file('/work/' + file.name);
        console.log("📤 File contents:", content);
        self.postMessage({ result: content });
      } catch (err) {
        console.error("❌ Error inside worker:", err);
        self.postMessage({ error: err.message });
      }
    };
  }
}).then((mod) => {
  Module = mod;
  console.log("📦 Module initialized");
});
