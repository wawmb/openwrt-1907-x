#!/bin/bash

function RED() {
    echo -e "\033[31m$@\033[0m"
}

function BLUE() {
    echo -e "\033[34m$@\033[0m"
}

function Select_Option() {
    choices=("$@")
    selected=1

    while true; do
        clear
        for index in "${!choices[@]}"; do
            if [ $index -eq 0 ]; then
                echo -e "\033[1m${choices[$index]}\033[0m"
            else
                if [ $index -eq $selected ]; then
                    printf "\033[31m> ${choices[$index]}\033[0m\n"
                else
                    echo "  ${choices[$index]}"
                fi
            fi
        done

        read -n1 -s key
        case "$key" in
        A)
            if [ $selected -gt 1 ]; then
                selected=$((selected - 1))
            fi
            ;;
        B)
            if [ $selected -lt $((${#choices[@]} - 1)) ]; then
                selected=$((selected + 1))
            fi
            ;;
        "")
            break
            ;;
        esac
    done

    selected_option="${choices[$selected]}"
}

function Obtain_Config_Info() {
    target_file="${script_dir}/configs/.target.cfg"
    if [ -s "$target_file" ]; then
        deconfig=$(cat "$target_file" | awk -F '|' '{gsub(/^[ \t]+|[ \t]+$/, "", $1); print $1}' | awk '{print $NF}')
        kernel_path=$(cat "$target_file" | awk -F '|' '{gsub(/^[ \t]+|[ \t]+$/, "", $1); gsub(/[ \t]+/, "", $2); print $2}')
        kernel_branch=$(cat "$target_file" | awk -F '|' '{gsub(/^[ \t]+|[ \t]+$/, "", $1); gsub(/[ \t]+/, "", $3); print $3}')
    else
        echo "File [.target.cfg] does not exist or is empty."
        exit 1
    fi
}

function Show_Info {
    local width=70
    local title=$1
    local border=$(printf "%${width}s" | tr ' ' '*')
    local sub_border=$(printf "%${width}s" | tr ' ' '-')
    local title_length=${#title}
    local padding=$(((width - title_length) / 2 - 2))
    local formatted_title=$(printf "%${padding}s%s%${padding}s" "" "$title" "")
    RED "${border}"
    BLUE "$formatted_title"
    BLUE "${sub_border}"
    printf "  %16s %s\n" "DECONFIG =" "${deconfig}"
    printf "  %16s %s\n" "KERNEL_PATH =" "${kernel_path}"
    printf "  %16s %s\n" "KERNEL_BRANCH =" "${kernel_branch}"
    RED "${border}"
}

function Switch_Kernel_Branch() {
    cd $script_dir
    path=$(realpath "$PWD/../$kernel_path")

    if [ -d "$path" ]; then
        cd $path
    else
        echo "Project [$path] does not exist."
        exit 1
    fi

    if [ "$kernel_branch" != "" ]; then
        # if ! git show-ref --quiet refs/heads/"$kernel_branch"; then #本地分支
        if ! git ls-remote --exit-code --heads origin "$kernel_branch" >/dev/null; then #远程分支
            echo "Specified branch [$kernel_branch] does not exist."
            exit 1
        fi
        git checkout "$kernel_branch"
        if [ $? -eq 0 ]; then
            echo "Switching to kernel: [$path] branch: [$kernel_branch] Succeeded."
        else
            echo "Git pull failed. Check the error message."
            exit 1
        fi
    fi
}

function Build_OpenWrt-X() {
    Obtain_Config_Info
    clear
    Show_Info "Start Build OpenWrt-X"
    cp $script_dir/configs/$deconfig $script_dir/.config
    Switch_Kernel_Branch
    Pull_All_Projects
    cd $script_dir
    make defconfig
    make -j $(nproc)
    if [ $? -eq 0 ]; then
        echo "======================= Build OpenWrt-X Succeeded ======================="
    else
        echo "================== Build OpenWrt-X Failed. Please Check =================="
        exit 1
    fi
}

function Pull_All_Projects() {
    cd $script_dir
    git branch
    git pull
    if [ $? -eq 0 ]; then
        echo "Git pull project [$PWD] succeeded."
    else
        echo "Git pull failed. Check the error message."
        exit 1
    fi

    kernel_path=$(realpath "$PWD/../$kernel_path")
    if [ -d "$kernel_path" ]; then
        cd $kernel_path
    else
        echo "Project [$kernel_path] does not exist."
        exit 1
    fi
    git branch
    git pull
    if [ $? -eq 0 ]; then
        echo "Git pull project [$PWD] succeeded."
    else
        echo "Git pull failed. Check the error message."
        exit 1
    fi

    ate_path=$(realpath "$script_dir/package/utils/auto-factory")
    if [ -d "$ate_path" ]; then
        cd $ate_path
    else
        echo "Project [$ate_path] does not exist."
        return 0
    fi
    git branch
    git pull
    if [ $? -eq 0 ]; then
        echo "Git pull project [$PWD] succeeded."
    else
        echo "Git pull failed. Check the error message."
        exit 1
    fi

    nf51xx_path=$(realpath "$script_dir/package/kernel/xspeed/nf51xx/src/nf51xx")
    if [ -d "$nf51xx_path" ]; then
        cd $nf51xx_path
    else
        echo "Project [$nf51xx_path] does not exist."
        return 0
    fi
    git branch
    git pull
    if [ $? -eq 0 ]; then
        echo "Git pull project [$PWD] succeeded."
    else
        echo "Git pull failed. Check the error message."
        exit 1
    fi
}

function Export_The_Build_Files() {
    subtarget=$(sed -n 's/^CONFIG_TARGET_SUBTARGET="\([^"]*\)"/\1/p' $script_dir/.config)
    build_path="$script_dir/bin/targets/xspeed/$subtarget-glibc"
    export_path=$(realpath "/data/chfsfile/board/$subtarget-glibc")
    echo -e "Export Path is \e[34m $export_path \e[0m"
    read -t 8 -e -p "Change the export path? [N|Y]" ANS || :
    ANS=${ANS:-n}

    case $ANS in
    Y | y | yes | YES | Yes)
        read -e -p "Please enter the new path: " path
        export_path=$path
        ;;
    N | n | no | NO | No) ;;
    *) ;;
    esac

    mkdir $export_path
    if [ ! -d "$build_path" ]; then
        echo "Path [$build_path] does not exist, Please verify."
        exit 1
    fi

    default_files=("auto-factory.bin" "cpio.gz" "vmlinux")
    specify=false
    current_path=$(pwd)
    echo -e "Build Path is \e[34m $build_path \e[0m"
    echo -e "Current Path is \e[34m $current_path \e[0m"
    echo -e "Files to copy are: \e[34m${default_files[@]}\e[0m"
    read -t 8 -e -p "Do you want to specify files? [N|Y]" ANS || :
    ANS=${ANS:-n}

    case $ANS in
    Y | y | yes | YES | Yes)
        read -e -p "Please enter the file names (separate by spaces): " -a custom_files
        files_to_copy=("${custom_files[@]}")
        specify=true
        ;;
    N | n | no | NO | No) files_to_copy=("${default_files[@]}") ;;
    *) ;;
    esac

    for file in "${files_to_copy[@]}"; do
        if $specify; then
            if [[ -f "$current_path/$file" ]]; then
                cp "$current_path/$file" "$export_path"
                echo "Copied (from current path): $file"
            else
                echo "File not found in current path: $file"
            fi
        else
            if [[ -f "$build_path/$file" ]]; then
                cp "$build_path/$file" "$export_path"
                echo "Copied (from build path): $file"
            else
                echo "File not found in build path: $file"
            fi
        fi
    done
}

function Sub_Menu() {
    menu_file="${script_dir}/configs/menu.cfg"
    mapfile -t sub_options <"$menu_file"

    while true; do
        Select_Option "${sub_options[@]}"
        case "$selected_option" in
        *"defconfig"*)
            echo "$selected_option" >"$script_dir/configs/.target.cfg"
            exit_main_menu=true
            break
            ;;
        *"Back to Main Menu")
            echo "Back to Main Menu"
            break
            ;;
        *) ;;
        esac
    done
}

function Main_Menu() {
    main_options=("======================================== Main Menu ========================================" "1. Build OpenWrt-X" "2. Pull All Projects" "3. Export The Build Files" "4. Exit Main Menu")

    while true; do
        Select_Option "${main_options[@]}"
        case "$selected_option" in
        *"Build OpenWrt-X")
            echo "Build OpenWrt-X"
            Sub_Menu
            if [ "$exit_main_menu" == "true" ]; then
                Build_OpenWrt-X
                break
            fi
            ;;
        *"Pull All Projects")
            Obtain_Config_Info
            Show_Info "Pull All Projects"
            Pull_All_Projects
            break
            ;;
        *"Export The Build Files")
            Obtain_Config_Info
            Show_Info "Export The Build Files"
            Export_The_Build_Files
            break
            ;;
        *"Exit Main Menu")
            echo "Exit Main Menu"
            break
            ;;
        *) ;;
        esac
    done
}

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exit_main_menu=false
deconfig=
kernel_path=
kernel_branch=
Main_Menu
