
import initModule from './build/dng_example.js';

let Module;
let imagent_canvas;
let first_time = true;
initModule({
  onRuntimeInitialized() {
    console.log("✅ WASM Module ready inside worker");
    // self.postMessage({ log: "✅ Worker is ready" });

    //trying multiple files
    self.onmessage = async function (e) {
      // const files = e.data.files;
      let { files, canvas, type, exposure  } = e.data;

      if (imagent_canvas === undefined || !imagent_canvas)
      {
        imagent_canvas = canvas
      }

      if (type === 'dcp') {


        const file = e.data.file;

        const arrayBuffer = await file.arrayBuffer();
        const uint8Array = new Uint8Array(arrayBuffer);

        Module.FS_createPath('/', 'profiles', true, true);
        Module.FS_createDataFile('/profiles', 'Canon_EOS_R6_Adobe_Standard.dcp', uint8Array, true, true);

        console.log('✅ DCP file loaded into Worker\'s MEMFS');
        return
      }
      try {
        if (first_time)
        Module.FS.mkdir('/work');
      } catch (err) {
        console.warn("📁 mkdir failed (maybe already exists)", err);
      }

      try {
        if (first_time)
        Module.FS.mount(Module.WORKERFS, {files: files, canvas: imagent_canvas}, '/work');
        // console.log("📨 Worker received files:", files.length);

        const startTime = performance.now(); // ⏱️ start the clock

        const results = [];
        for (const file of files) {
          try {
            // console.log("📨 file:", file);
            if (exposure === undefined) exposure =0;
            const vec = new Module.VectorUint8();

            Module.read_file('/work/' + file.name,vec, exposure);
            const result = vec;
        // Extract bytes manually
            const size = result.size();
            const rawArray = [];

            for (let i = 0; i < size; i++) {
              rawArray.push(result.get(i));
            }

            const byteArray = new Uint8Array(rawArray);

            const blob = new Blob([byteArray], { type: 'image/jpeg' });
            const imageBitmap = await createImageBitmap(blob);

            const ctx = imagent_canvas.getContext('2d');
            ctx.clearRect(0, 0, imageBitmap.width, imageBitmap.height);
            imagent_canvas.width = imageBitmap.width;
            imagent_canvas.height = imageBitmap.height;
            ctx.drawImage(imageBitmap, 0, 0);

            first_time = false
            results.push({file: file.name, result});
          } catch (err) {
            console.log({"error": err})
            results.push({file: file.name, error: err.message});
          }
        }
        const endTime = performance.now(); // ⏱️ stop the clock
        // const totalTime = endTime - startTime;

        // self.postMessage({type: "log", log: `Total time: ${totalTime.toFixed(2)} ms`});

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
