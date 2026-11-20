#!/usr/bin/env bash
# Cắt logo / icon / hình xe từ ảnh thiết kế (assets/design.jpg), tách nền tối,
# xuất thành mảng ảnh LVGL: src/images/design_assets.c
#
# Chạy trên Windows (Git Bash) – dùng Microsoft Edge ở chế độ headless để xử lý ảnh bằng canvas.
# Danh sách vùng cắt và kích thước nằm trong tools/assets.js.
set -euo pipefail
cd "$(dirname "$0")/.."

EDGE="${EDGE:-/c/Program Files (x86)/Microsoft/Edge/Application/msedge.exe}"
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

cp tools/assets.js "$WORK/assets.js"
B64=$(base64 -w0 assets/design.jpg)

cat > "$WORK/extract.html" <<EOF
<!doctype html><html><head><meta charset="utf-8"></head><body><pre id="out"></pre>
<script src="assets.js"></script><script>
const img = new Image();
img.onload = () => {
  let s = '/* Tạo tự động bởi tools/extract_assets.sh từ assets/design.jpg – không sửa tay */\n#include "lvgl.h"\n\n';
  for (const a of ASSETS) s += toLvglC(a.name, extractAsset(img, a));
  out.textContent = '@@' + 'BEGIN@@' + s + '@@' + 'END@@';
};
img.src = 'data:image/jpeg;base64,$B64';
</script></body></html>
EOF

W=$(cygpath -m "$WORK")
"$EDGE" --headless=new --disable-gpu --virtual-time-budget=5000 --allow-file-access-from-files \
  --dump-dom "file:///$W/extract.html" 2>/dev/null > "$WORK/dom.txt"

mkdir -p src/images
awk 'BEGIN{p=0} /@@BEGIN@@/{p=1; sub(/.*@@BEGIN@@/,"")} p{ if (index($0,"@@END@@")) { sub(/@@END@@.*/,""); print; exit } print }' \
  "$WORK/dom.txt" | sed 's/&quot;/"/g; s/&lt;/</g; s/&gt;/>/g; s/&amp;/\&/g' > src/images/design_assets.c

echo "Đã tạo src/images/design_assets.c ($(grep -c lv_img_dsc_t src/images/design_assets.c) ảnh)"
