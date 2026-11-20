// Danh sách chi tiết cắt từ ảnh thiết kế (esp32/assets/design.jpg, 1448x1086)
// rect = [x, y, w, h] trong ảnh gốc; out = [w, h] kích thước trên màn 800x480
// Nền tối được tách ra (alpha theo độ chênh so với màu nền ở viền vùng cắt).
const ASSETS = [
  { name: 'img_logo_small',  rect: [72, 60, 92, 80],     out: [42, 36] },
  { name: 'img_logo_center', rect: [652, 752, 152, 142], out: [78, 73] },
  { name: 'img_bluetooth',   rect: [978, 64, 56, 70],    out: [24, 30] },
  { name: 'img_road',        rect: [1044, 218, 76, 72],  out: [40, 38] },
  { name: 'img_thermo',      rect: [1048, 544, 64, 100], out: [32, 50] },
  { name: 'img_bolt',        rect: [50, 734, 106, 106],  out: [54, 54] },
  { name: 'img_plug',        rect: [1008, 720, 66, 80],  out: [34, 41] },
  { name: 'img_current',     rect: [1004, 834, 76, 76],  out: [39, 39] },
  { name: 'img_scooter',     rect: [106, 498, 242, 164], out: [148, 100] },
  { name: 'img_leaf_l',      rect: [508, 978, 50, 46],   out: [24, 22] },
  { name: 'img_leaf_r',      rect: [890, 978, 50, 46],   out: [24, 22] },
];

// Cắt + thu nhỏ + tách nền. Trả về ImageData (RGBA, alpha thẳng)
function extractAsset(img, a) {
  const [x, y, w, h] = a.rect, [ow, oh] = a.out;
  const c = document.createElement('canvas');
  c.width = ow; c.height = oh;
  const g = c.getContext('2d');
  g.imageSmoothingEnabled = true;
  g.imageSmoothingQuality = 'high';
  g.drawImage(img, x, y, w, h, 0, 0, ow, oh);
  const d = g.getImageData(0, 0, ow, oh);
  const p = d.data;

  // màu nền = trung bình viền vùng cắt
  let br = 0, bg = 0, bb = 0, n = 0;
  for (let i = 0; i < ow; i++) for (const j of [0, oh - 1]) { const k = (j * ow + i) * 4; br += p[k]; bg += p[k + 1]; bb += p[k + 2]; n++; }
  for (let j = 0; j < oh; j++) for (const i of [0, ow - 1]) { const k = (j * ow + i) * 4; br += p[k]; bg += p[k + 1]; bb += p[k + 2]; n++; }
  br /= n; bg /= n; bb /= n;

  const K = 110;   // độ chênh để coi là đặc hoàn toàn
  for (let k = 0; k < p.length; k += 4) {
    const dr = p[k] - br, dg = p[k + 1] - bg, db = p[k + 2] - bb;
    const dist = Math.max(dr, dg, db, 0);
    let al = Math.min(1, dist / K);
    al = al < 0.06 ? 0 : al;
    if (al > 0) {
      // bỏ trộn với nền: c = bg + (c - bg) / alpha
      p[k] = Math.min(255, Math.max(0, br + dr / al));
      p[k + 1] = Math.min(255, Math.max(0, bg + dg / al));
      p[k + 2] = Math.min(255, Math.max(0, bb + db / al));
    }
    p[k + 3] = Math.round(al * 255);
  }
  return d;
}

// ImageData → mảng C cho LVGL 8 (LV_IMG_CF_TRUE_COLOR_ALPHA, RGB565 little-endian + alpha)
function toLvglC(name, d) {
  const p = d.data, bytes = [];
  for (let k = 0; k < p.length; k += 4) {
    const v = ((p[k] >> 3) << 11) | ((p[k + 1] >> 2) << 5) | (p[k + 2] >> 3);
    bytes.push(v & 0xFF, v >> 8, p[k + 3]);
  }
  let s = `static const LV_ATTRIBUTE_MEM_ALIGN uint8_t ${name}_map[] = {\n`;
  for (let i = 0; i < bytes.length; i += 24) {
    s += '  ' + bytes.slice(i, i + 24).map(b => '0x' + b.toString(16).padStart(2, '0')).join(',') + ',\n';
  }
  s += '};\n\n';
  s += `const lv_img_dsc_t ${name} = {\n` +
       `  .header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA,\n  .header.always_zero = 0,\n  .header.reserved = 0,\n` +
       `  .header.w = ${d.width},\n  .header.h = ${d.height},\n` +
       `  .data_size = ${bytes.length},\n  .data = ${name}_map,\n};\n\n`;
  return s;
}
