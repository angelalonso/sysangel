#!/bin/bash
input_file="$1"

if [ ! -f "$input_file" ]; then
    echo "File not found: $input_file"
    exit 1
fi

echo "Processing $input_file..."
count=0

while IFS= read -r line; do
    if [[ "$line" == Path1:* ]]; then
        path="${line#Path1: }"
        if [ -e "$path" ]; then
            echo "Removing: $path"
            rm -- "$path"
            ((count++))
        else
            echo "Not found: $path"
        fi
    fi
done < "$input_file"

echo "Done. Removed $count files."
