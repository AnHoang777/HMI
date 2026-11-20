#!/usr/bin/env bash
# Tạo font LVGL (có tiếng Việt) từ Be Vietnam Pro (giấy phép SIL OFL).
# Chạy trong GitHub Actions trước khi build. Cần Node.js và curl.
set -euo pipefail
cd "$(dirname "$0")/.."

TTF=.fonts
OUT=src/fonts
mkdir -p "$TTF" "$OUT"

BASE=https://raw.githubusercontent.com/google/fonts/main/ofl/bevietnampro
for f in ExtraBold Bold SemiBold Medium BlackItalic; do
  [ -f "$TTF/BeVietnamPro-$f.ttf" ] || curl -fsSL -o "$TTF/BeVietnamPro-$f.ttf" "$BASE/BeVietnamPro-$f.ttf"
done

# ASCII + ° + toàn bộ chữ cái tiếng Việt có dấu + … + ●
VN="0x20-0x7E,0xB0,0xC0-0xC3,0xC8-0xCA,0xCC-0xCD,0xD2-0xD5,0xD9-0xDA,0xDD,0xE0-0xE3,0xE8-0xEA,0xEC-0xED,0xF2-0xF5,0xF9-0xFA,0xFD,0x102-0x103,0x110-0x111,0x128-0x129,0x168-0x169,0x1A0-0x1A1,0x1AF-0x1B0,0x1EA0-0x1EF9,0x2026,0x25CF"

# Tên biến font = tên file đầu ra (font_xxx.c → font_xxx)
conv() {
  local ttf=$1 size=$2 name=$3
  shift 3
  local log
  if ! log=$(npx --yes lv_font_conv@1.5.2 --no-compress --bpp 4 --format lvgl \
      --font "$TTF/$ttf" --size "$size" "$@" -o "$OUT/$name.c" 2>&1); then
    # in lỗi thành annotation để xem được trên trang GitHub Actions
    echo "::error title=lv_font_conv $name::$(echo "$log" | tail -n 5 | tr '\n' ' ')"
    echo "$log"
    exit 1
  fi
  echo "  $name.c ($(wc -c < "$OUT/$name.c") bytes)"
}

conv BeVietnamPro-ExtraBold.ttf   120 font_speed_120 --symbols "0123456789-"
conv BeVietnamPro-BlackItalic.ttf  54 font_soc_54    --symbols "0123456789%-"
conv BeVietnamPro-ExtraBold.ttf    46 font_range_46  --symbols "0123456789-"
conv BeVietnamPro-Bold.ttf         32 font_value_32  --range "0x20-0x7E,0xB0"
conv BeVietnamPro-SemiBold.ttf     24 font_mid_24    --range "$VN"
conv BeVietnamPro-Medium.ttf       18 font_label_18  --range "$VN"
