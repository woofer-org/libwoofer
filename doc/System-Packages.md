# System dependency package reference

This is an overview of what packages are needed on what type of systems and how
to install these packages using the command-line.

Please note that the listed packages are incremental, meaning the packages
needed for compilation include those needed for runtime and the ones needed for
development include those needed for compilation and runtime (although the
packages needed e.g. for compilation usually depend on those needed for
runtime and are thus installed automatically).

## Debian based (including Ubuntu)

### Runtime

```sh
sudo apt install libglib2.0-0 libgstreamer1.0-0 libgstreamer-plugins-base1.0-0 libgstreamer-plugins-good1.0-0
```

### Compilation

```sh
sudo apt install gcc make pkg-config libglib2.0-dev libgstreamer1.0-dev
```

### Development

```sh
sudo apt install autoconf
```

### Documentation

```sh
sudo apt install gcc-doc make-doc libglib2.0-doc gstreamer1.0-doc
```

### Optional

```sh
sudo apt install libgstreamer-plugins-base1.0-dev libgstreamer-plugins-good1.0-dev
```

## Red Hat based (including Fedora)

### Runtime

```sh
sudo dnf install glib gstreamer gstreamer1-plugins-base gstreamer1-plugins-good
```

### Compilation

```sh
sudo dnf install gcc make pkg-config glib-devel gstreamer1-devel
```

### Development

```sh
sudo dnf install autoconf
```

### Documentation

```sh
sudo dnf install gcc-doc make-doc glib-doc gstreamer1-doc
```

### Optional

```sh
sudo dnf install gstreamer1-plugins-base-devel gstreamer1-plugins-good-devel
```

## Arch Linux based (including Manjaro)

### Runtime

```sh
sudo pacman -S glib2 gstreamer
```

### Compilation

```sh
sudo pacman -S gcc make pkg-config
```

### Development

```sh
sudo pacman -S autoconf
```

### Documentation

```sh
sudo pacman -S glib2-docs gstreamer-docs
```

## openSUSE

### Runtime

```sh
sudo zypper install glib2 gstreamer gstreamer-plugins-base gstreamer-plugins-good
```

### Compilation

```sh
sudo zypper install gcc make pkg-config glib2-devel gstreamer-devel
```

### Development

```sh
sudo zypper install autoconf glib2-tools
```

### Documentation

```sh
sudo zypper install gcc-info gstreamer-docs gstreamer-plugins-base-docs gstreamer-plugins-good-docs
```

### Optional

```sh
sudo zypper install gstreamer-plugins-base-devel gstreamer-plugins-good-devel
```

