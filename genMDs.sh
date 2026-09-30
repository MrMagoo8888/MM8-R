find . -type f \
  ! -path "./docs/*" \
  ! -path "./.git/*" \
  ! -path "./build/*" \
  ! -path "./toolchain/*" \
  ! -path "./dist/*" \
  ! -name "qlog.txt" \
  -exec bash -c '
    for file do
      # Strip leading "./" to get clean relative path
      rel_path="${file#./}"
      target_docs_file="docs/projectRoot/${rel_path}.md"
      
      # Create directory structure inside docs if missing
      mkdir -p "$(dirname "$target_docs_file")"
      
      # If the markdown file does not exist yet, create it with a template
      if [ ! -f "$target_docs_file" ]; then
        echo -e "# Documentation: $rel_path\n\n## Overview\nAdd file description here...\n\n## Architecture / Notes\n- Component:\n- Dependencies:" > "$target_docs_file"
        echo "Created template for: $rel_path"
      fi
    done
  ' _ {} +
