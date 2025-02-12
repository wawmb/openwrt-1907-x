![OpenWrt logo](/logo.svg)

## 编译命令

1. 首先装好 Linux 系统，推荐 Debian 11 或 Ubuntu LTS

2. 安装编译依赖

   ```bash
   sudo apt update -y
   sudo apt full-upgrade -y
   sudo apt install -y ack antlr3 asciidoc autoconf automake autopoint binutils bison build-essential \
   bzip2 ccache cmake cpio curl device-tree-compiler fastjar flex gawk gettext gcc-multilib g++-multilib \
   git gperf haveged help2man intltool libc6-dev-i386 libelf-dev libfuse-dev libglib2.0-dev libgmp3-dev \
   libltdl-dev libmpc-dev libmpfr-dev libncurses5-dev libncursesw5-dev libpython3-dev libreadline-dev \
   libssl-dev libtool lrzsz mkisofs msmtp ninja-build p7zip p7zip-full patch pkgconf python3 \
   python3-pyelftools python3-setuptools qemu-utils rsync scons squashfs-tools subversion swig texinfo \
   uglifyjs upx-ucl unzip vim wget xmlto xxd zlib1g-dev
   ```

3. 下载源代码

   ```bash
   git clone https://github.com/wawmb/openwrt-1907-x.git
   cd openwrt-1907-x
   ```

4. 更新 feeds

   ```bash
   ./scripts/feeds update -a  
   ./scripts/feeds install -a 
   ```

5. 使用编译脚本并选择配置

   ```bash
   ./build.sh
   ```

### 使用 WSL/WSL2 进行编译

由于 WSL 的 PATH 中包含带有空格的 Windows 路径，有可能会导致编译失败，请在 `make` 前面加上：

```bash
PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
```

由于默认情况下，装载到 WSL 发行版的 NTFS 格式的驱动器将不区分大小写，因此大概率在 WSL/WSL2 的编译检查中会返回以下错误：

```txt
Build dependency: OpenWrt can only be built on a case-sensitive filesystem
```

一个比较简洁的解决方法是，在 `git clone` 前先创建 Repository 目录，并为其启用大小写敏感：

```powershell
# 以管理员身份打开终端
PS > fsutil.exe file setCaseSensitiveInfo <your_local_path> enable
# 将本项目 git clone 到开启了大小写敏感的目录 <your_local_path> 中
PS > git clone https://github.com/wawmb/openwrt-1907-x.git <your_local_path>
```

> 对已经 `git clone` 完成的项目目录执行 `fsutil.exe` 命令无法生效，大小写敏感只对新增的文件变更有效。

### macOS 原生系统进行编译

1. 在 AppStore 中安装 Xcode

2. 安装 Homebrew：

   ```bash
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   ```

3. 使用 Homebrew 安装工具链、依赖与基础软件包：

   ```bash
   brew unlink awk
   brew install coreutils diffutils findutils gawk gnu-getopt gnu-tar grep make ncurses pkg-config wget quilt xz
   brew install gcc@11
   ```

4. 然后输入以下命令，添加到系统环境变量中：

   - intel 芯片的 mac

   ```bash
   echo 'export PATH="/usr/local/opt/coreutils/libexec/gnubin:$PATH"' >> ~/.bashrc
   echo 'export PATH="/usr/local/opt/findutils/libexec/gnubin:$PATH"' >> ~/.bashrc
   echo 'export PATH="/usr/local/opt/gnu-getopt/bin:$PATH"' >> ~/.bashrc
   echo 'export PATH="/usr/local/opt/gnu-tar/libexec/gnubin:$PATH"' >> ~/.bashrc
   echo 'export PATH="/usr/local/opt/grep/libexec/gnubin:$PATH"' >> ~/.bashrc
   echo 'export PATH="/usr/local/opt/gnu-sed/libexec/gnubin:$PATH"' >> ~/.bashrc
   echo 'export PATH="/usr/local/opt/make/libexec/gnubin:$PATH"' >> ~/.bashrc
   ```

   - apple 芯片的 mac

   ```zsh
   echo 'export PATH="/opt/homebrew/opt/coreutils/libexec/gnubin:$PATH"' >> ~/.bashrc
   echo 'export PATH="/opt/homebrew/opt/findutils/libexec/gnubin:$PATH"' >> ~/.bashrc
   echo 'export PATH="/opt/homebrew/opt/gnu-getopt/bin:$PATH"' >> ~/.bashrc
   echo 'export PATH="/opt/homebrew/opt/gnu-tar/libexec/gnubin:$PATH"' >> ~/.bashrc
   echo 'export PATH="/opt/homebrew/opt/grep/libexec/gnubin:$PATH"' >> ~/.bashrc
   echo 'export PATH="/opt/homebrew/opt/gnu-sed/libexec/gnubin:$PATH"' >> ~/.bashrc
   echo 'export PATH="/opt/homebrew/opt/make/libexec/gnubin:$PATH"' >> ~/.bashrc
   ```

5. 重新加载一下 shell 启动文件 `source ~/.bashrc`，然后输入 `bash` 进入 bash shell，就可以和 Linux 一样正常编译了

## 特别提示

1. **不要用 root 用户进行编译**
2. 国内用户编译前最好准备好梯子