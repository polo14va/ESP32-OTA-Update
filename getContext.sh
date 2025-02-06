#!/bin/bash
root_dir="."
output_file="context.files"

ignore_files=("getContext.sh" "Podfile.lock" "Cartfile.resolved")
ignore_dirs=("build" "Pods" "Carthage" ".swiftpm" ".idea" ".vscode" ".xcodeproj" ".sh" "AppIcon.appiconset")
ignore_exts=("xcassets" "xcuserstate" "xcworkspace" "png" "jpg" "jpeg" "gif" "svg" "pdf" "zip" "tar" "gz" "json" "plist")
temp_files=("context.files.tmp" "temp_tree.txt" "temp_tree_paths.txt" "temp_dirs.txt")

rm -f "$output_file"

if ! command -v git >/dev/null 2>&1; then
  echo "Error: Git is not installed."
  exit 1
fi

cd "$root_dir" || exit

if [ -d ".git" ]; then
  files=$(git ls-files --cached --others --exclude-standard | sed 's|^\./||')
else
  files=$(find . -type f | grep -Ev '(^|/)\.' | sed 's|^\./||')
fi

for pattern in "${temp_files[@]}"; do
  files=$(echo "$files" | grep -v "^$pattern$")
done
for pattern in "${ignore_files[@]}"; do
  files=$(echo "$files" | grep -v "^$pattern$")
done
for pattern in "${ignore_dirs[@]}"; do
  files=$(echo "$files" | grep -v "^$pattern/")
done

{
  echo "File tree and content:"
  echo ""
  echo "Directory and file tree:"
} > "$output_file"

dir_list=$(echo "$files" | xargs -n1 dirname | sort -u)
all_paths=$(echo -e "$dir_list\n$files" | sort)

while IFS= read -r path; do
  trimmed="${path#./}"
  level=$(echo "$trimmed" | awk -F'/' '{print NF-1}')
  indent=$(printf '  %.0s' $(seq 1 $level))
  name=$(basename "$trimmed")
  echo "${indent}- $name" >> "$output_file"
done <<< "$all_paths"

file_count=$(echo "$files" | wc -l | tr -d ' ')
{
  echo ""
  echo "Total files: $file_count"
  echo ""
  echo "File contents:"
} >> "$output_file"

while IFS= read -r file; do
  {
    echo ""
    echo "Path: $file"
  } >> "$output_file"
  ext="${file##*.}"
  skip_content=false
  for ext_ignore in "${ignore_exts[@]}"; do
    if [ "$ext" = "$ext_ignore" ]; then
      skip_content=true
      break
    fi
  done
  if [ "$skip_content" = false ]; then
    {
      echo ""
      if [ -f "$file" ]; then
        cat "$file"
      else
        echo "File $file does not exist."
      fi
      echo ""
    } >> "$output_file"
  fi
done <<< "$files"

pbcopy < "$output_file"
rm "$output_file"

echo ""
echo "Copied $file_count files to clipboard."