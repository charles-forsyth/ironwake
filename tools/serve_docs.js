// Tiny static server for local checks of docs/ (correct MIME for .glb).
// Usage: node tools/serve_docs.js [port]
const http = require("http");
const fs = require("fs");
const path = require("path");

const root = path.resolve(__dirname, "..", "docs");
const port = Number(process.argv[2] || 8765);
const MIME = {
  ".html": "text/html; charset=utf-8", ".css": "text/css", ".js": "text/javascript",
  ".json": "application/json", ".glb": "model/gltf-binary", ".jpg": "image/jpeg",
  ".png": "image/png", ".mp4": "video/mp4", ".mp3": "audio/mpeg", ".gif": "image/gif"
};

http.createServer((req, res) => {
  let p = decodeURIComponent(req.url.split("?")[0]);
  if (p.endsWith("/")) p += "index.html";
  const f = path.join(root, p);
  if (!f.startsWith(root)) { res.writeHead(403); return res.end(); }
  fs.stat(f, (err, st) => {
    if (err || !st.isFile()) { res.writeHead(404); return res.end("not found"); }
    const type = MIME[path.extname(f)] || "application/octet-stream";
    const range = req.headers.range;
    if (range) {
      const [a, b] = range.replace("bytes=", "").split("-");
      const start = Number(a), end = b ? Number(b) : st.size - 1;
      res.writeHead(206, { "Content-Type": type, "Content-Range": `bytes ${start}-${end}/${st.size}`, "Accept-Ranges": "bytes", "Content-Length": end - start + 1 });
      return fs.createReadStream(f, { start, end }).pipe(res);
    }
    res.writeHead(200, { "Content-Type": type, "Content-Length": st.size, "Accept-Ranges": "bytes" });
    fs.createReadStream(f).pipe(res);
  });
}).listen(port, "127.0.0.1", () => console.log(`serving ${root} on http://127.0.0.1:${port}/`));
