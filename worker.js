
import initModule from './build/dng_example.js';

let Module;

initModule({
  onRuntimeInitialized() {
    console.log("✅ WASM Module ready inside worker");
    // self.postMessage({ log: "✅ Worker is ready" });

    //trying multiple files
    self.onmessage = async function (e) {
      // const files = e.data.files;
      const { files, canvas, type } = e.data;
      console.log("1111", e.data)
      if (type === 'dcp') {

        const file = e.data.file;
        console.log("file111", file)

        const arrayBuffer = await file.arrayBuffer();
        const uint8Array = new Uint8Array(arrayBuffer);

        Module.FS_createPath('/', 'profiles', true, true);
        Module.FS_createDataFile('/profiles', 'Canon_EOS_R6_Adobe_Standard.dcp', uint8Array, true, true);

        console.log('✅ DCP file loaded into Worker\'s MEMFS');
        return
      }
      try {
        Module.FS.mkdir('/work');
      } catch (err) {
        console.warn("📁 mkdir failed (maybe already exists)", err);
      }

      try {

        Module.FS.mount(Module.WORKERFS, {files: files, canvas: canvas}, '/work');
        // console.log("📨 Worker received files:", files.length);

        const startTime = performance.now(); // ⏱️ start the clock

        const results = [];
        console.log("files", files)
        for (const file of files) {
          try {

            const result = Module.read_file('/work/' + file.name);
            console.log("result",result);
// Extract bytes manually
            const size = result.size();
            const rawArray = [];

            for (let i = 0; i < size; i++) {
              rawArray.push(result.get(i));
            }

            console.log(rawArray);            // Create Blob from result
            const byteArray = new Uint8Array(rawArray);

            const blob = new Blob([byteArray], { type: 'image/jpeg' });
            const imageBitmap = await createImageBitmap(blob);
          // Now draw it
            const ctx = canvas.getContext('2d');
            canvas.width = imageBitmap.width;
            canvas.height = imageBitmap.height;
            ctx.drawImage(imageBitmap, 0, 0);


            results.push({file: file.name, result});
          } catch (err) {
            console.log({"error": err})
            results.push({file: file.name, error: err.message});
          }
        }
        const endTime = performance.now(); // ⏱️ stop the clock
        const totalTime = endTime - startTime;

        self.postMessage({type: "log", log: `Total time: ${totalTime.toFixed(2)} ms`});

      } catch (err) {
        console.error("❌ Error mounting or reading:", err);
        self.postMessage({error: err.message});
      }
    };

  }
}).then((mod) => {
  Module = mod;
  console.log("📦 Module initialized");
});
