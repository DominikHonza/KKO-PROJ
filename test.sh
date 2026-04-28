#!/bin/bash

set -e

BIN=./lz_codec
DATA_DIR=./data
TMP_DIR=./tmp
RESULTS=results.csv
MD_RESULTS=results.md

cleanup() {
    echo ""
    echo "Cleaning tmp..."
    rm -rf "$TMP_DIR"
}
trap cleanup EXIT

rm -rf "$TMP_DIR"
mkdir -p "$TMP_DIR"
rm -f "$RESULTS" "$MD_RESULTS"

echo "Build..."
make

echo "file;mode;orig_bytes;comp_bytes;ratio;bpp;entropy;compress_time;decompress_time;status" >> $RESULTS

echo ""
echo "🚀 Running full test suite..."
echo "========================================"

MODES=("" "-m" "-a" "-a -m")

# Entropy (Shannon)
compute_entropy() {
    file=$1
    total=$(stat -c%s "$file")

    od -An -t u1 "$file" | tr -s ' ' '\n' | grep -v '^$' | \
    awk '
    {freq[$1]++}
    END {
        for (i in freq) {
            p = freq[i] / '"$total"'
            H += -p * log(p)/log(2)
        }
        printf "%.4f", H
    }'
}

for file in $DATA_DIR/*.raw; do
    filename=$(basename "$file")
    name="${filename%.*}"

    echo ""
    echo "Entropy: $filename"
    entropy=$(compute_entropy "$file")

    for mode in "${MODES[@]}"; do
        mode_label=$(echo $mode | tr ' ' '_' )
        [ -z "$mode_label" ] && mode_label="base"

        compressed="$TMP_DIR/${name}_${mode_label}.lz"
        decompressed="$TMP_DIR/${name}_${mode_label}_out.raw"

        echo "$filename | $mode_label"

        WIDTH=256

        # ⏱️ Komprese
        start=$(date +%s%N)
        $BIN -c $mode -i "$file" -o "$compressed" -w $WIDTH
        end=$(date +%s%N)
        comp_time=$(echo "scale=4; ($end - $start)/1000000000" | bc)

        # ⏱️ Dekomprese
        start=$(date +%s%N)
        $BIN -d -i "$compressed" -o "$decompressed"
        end=$(date +%s%N)
        decomp_time=$(echo "scale=4; ($end - $start)/1000000000" | bc)

        # ✅ Kontrola
        if cmp -s "$file" "$decompressed"; then
            status="OK"
        else
            status="FAIL"
        fi

        orig_size=$(stat -c%s "$file")
        comp_size=$(stat -c%s "$compressed")

        ratio=$(awk "BEGIN {printf \"%.4f\", $comp_size/$orig_size}")
        bpp=$(awk "BEGIN {printf \"%.4f\", ($comp_size*8)/$orig_size}")

        # CZ format
        ratio=$(echo "$ratio" | sed 's/\./,/')
        bpp=$(echo "$bpp" | sed 's/\./,/')
        entropy_fmt=$(echo "$entropy" | sed 's/\./,/')
        comp_time=$(echo "$comp_time" | sed 's/\./,/')
        decomp_time=$(echo "$decomp_time" | sed 's/\./,/')

        echo "$filename;$mode_label;$orig_size;$comp_size;$ratio;$bpp;$entropy_fmt;$comp_time;$decomp_time;$status" >> $RESULTS
    done
done

# CSV → Markdown
{
echo "| File | Mode | Orig (B) | Comp (B) | Ratio | BPP | Entropy | C Time | D Time | Status |"
echo "|------|------|----------|----------|--------|------|----------|---------|---------|--------|"

tail -n +2 $RESULTS | while IFS=';' read file mode orig comp ratio bpp entropy ct dt status; do
    echo "| $file | $mode | $orig | $comp | $ratio | $bpp | $entropy | $ct | $dt | $status |"
done
} > $MD_RESULTS

# Inject do README (bez duplikace)
awk '
BEGIN {inside=0}
/<!-- RESULTS_START -->/ {
    print;
    system("cat results.md");
    inside=1;
    next
}
/<!-- RESULTS_END -->/ {
    inside=0;
}
!inside
' README.md > README_tmp.md && mv README_tmp.md README.md

# SUMMARY
echo ""
echo "SUMMARY (average per mode)"
echo "========================================"

awk -F';' '
NR > 1 {
    mode = $2

    ratio = $5; gsub(",", ".", ratio)
    bpp = $6; gsub(",", ".", bpp)
    entropy = $7; gsub(",", ".", entropy)
    ct = $8; gsub(",", ".", ct)
    dt = $9; gsub(",", ".", dt)

    count[mode]++
    sum_ratio[mode] += ratio
    sum_bpp[mode] += bpp
    sum_entropy[mode] += entropy
    sum_ct[mode] += ct
    sum_dt[mode] += dt
}
END {
    printf "%-10s | %-8s | %-8s | %-10s | %-10s | %-10s\n", "Mode", "Ratio", "BPP", "Entropy", "C Time", "D Time"
    printf "--------------------------------------------------------------------------\n"

    for (m in count) {
        printf "%-10s | %-8.4f | %-8.4f | %-10.4f | %-10.4f | %-10.4f\n",
            m,
            sum_ratio[m]/count[m],
            sum_bpp[m]/count[m],
            sum_entropy[m]/count[m],
            sum_ct[m]/count[m],
            sum_dt[m]/count[m]
    }
}
' $RESULTS

echo ""
echo "========================================"
echo "Done"
echo "CSV: $RESULTS"
echo "README updated"