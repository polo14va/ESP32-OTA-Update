#!/bin/bash

# Variables de configuración
root_directory="."  # Directorio raíz del proyecto
output_file="context.files"  # Archivo de salida

# Lista de archivos a ignorar por petición propia (rutas relativas)
user_ignore_files=("getContext.sh" "Podfile.lock" "Cartfile.resolved")  # Agrega más archivos si es necesario

# Lista de directorios a ignorar por petición propia (rutas relativas)
user_ignore_dirs=("build" "Pods" "Carthage" ".swiftpm" ".idea" ".vscode" ".xcodeproj" ".sh" "AppIcon.appiconset") 

# Extensiones de archivos cuyo contenido no queremos mostrar
extensions_to_ignore=("xcassets" "xcuserstate" "xcworkspace" "png" "jpg" "jpeg" "gif" "svg" "pdf" "zip" "tar" "gz" "json" "plist")

# Archivos temporales creados por el script
temp_files=("context.files.tmp" "temp_tree.txt" "temp_tree_paths.txt" "temp_dirs.txt")

# Eliminar el archivo de salida si ya existe
rm -f "$output_file"

# Verificar que Git esté instalado
if ! command -v git >/dev/null 2>&1; then
    echo "Error: Git no está instalado."
    exit 1
fi

cd "$root_directory" || exit

# Obtener la lista de todos los archivos, excluyendo los ignorados por Git y archivos ocultos
file_list=$(echo "$file_list" | grep -v '^\./\.')

# Excluir archivos ocultos y archivos en directorios ocultos
file_list=$(echo "$file_list" | grep -v '^\.')
file_list=$(echo "$file_list" | grep -v '/\.')

# Excluir archivos temporales creados por el script
for temp_file in "${temp_files[@]}"; do
    file_list=$(echo "$file_list" | grep -v "^$temp_file$")
done

# Excluir archivos definidos en user_ignore_files
for ignore_file in "${user_ignore_files[@]}"; do
    file_list=$(echo "$file_list" | grep -v "^$ignore_file$")
done

# Excluir archivos en directorios definidos en user_ignore_dirs
for ignore_dir in "${user_ignore_dirs[@]}"; do
    file_list=$(echo "$file_list" | grep -v "^$ignore_dir/")
done

# Escribir el prompt inicial en el archivo de salida
echo "A continuación se muestra el árbol de archivos del proyecto y su contenido." > "$output_file"

echo "" >> "$output_file"
echo "Árbol de directorios y archivos:" >> "$output_file"

# Obtener la lista de directorios y archivos combinados
dir_list=$(echo "$file_list" | xargs -n1 dirname | sort -u)
all_paths=$(echo -e "$dir_list\n$file_list" | sort)

# Imprimir el árbol de directorios y archivos
echo "$all_paths" | while read -r path; do
    # Eliminar './' del inicio
    path="${path#./}"
    # Contar el número de '/' en la ruta para determinar el nivel
    level=$(echo "$path" | awk -F'/' '{print NF-1}')
    indent=$(printf '  %.0s' $(seq 1 $level))
    # Obtener el nombre base
    name=$(basename "$path")
    echo "${indent}- $name" >> "$output_file"
done

# Mostrar estadísticas de número de archivos
file_count=$(echo "$file_list" | wc -l)
echo "" >> "$output_file"
echo "Número total de archivos: $file_count" >> "$output_file"

# Ahora, imprimir el contenido de los archivos
echo "" >> "$output_file"
echo "Contenido de los archivos:" >> "$output_file"

echo "$file_list" | while read -r file; do
    # Escribir la ruta en el archivo de salida
    echo "" >> "$output_file"
    echo "Path: $file" >> "$output_file"

    # Obtener la extensión del archivo
    extension="${file##*.}"

    # Verificar si debemos ignorar el contenido del archivo
    skip_content=false

    # Comprobar si la extensión está en la lista de exclusiones
    for ext in "${extensions_to_ignore[@]}"; do
        if [[ "$extension" == "$ext" ]]; then
            skip_content=true
            break
        fi
    done

    # Si no debemos ignorar el contenido, agregarlo al archivo de salida
    if [ "$skip_content" = false ]; then
        echo "" >> "$output_file"
        if [ -f "$file" ]; then
            cat "$file" >> "$output_file"
        else
            echo "El archivo $file no existe." >> "$output_file"
        fi
        echo "" >> "$output_file"
    fi
done

# Copiar el contenido del archivo de salida al portapapeles
pbcopy < "$output_file"
rm "$output_file"

echo ""
echo "Copiados $file_count archivos al portapapeles."
