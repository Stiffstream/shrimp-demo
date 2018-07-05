# What Is Shrimp?

*Shrimp* is a small HTTP-server that provides shrunk images
generated from a specified set.

## Basic idea

Suppose you want your server to serve resized images over HTTP.
You receive a request and use its path to determine source image and
use request query string to determine resize parameters for it.
So you end up with the following form requests to your server:

* `http://myserver/path/to/image/img.jpg?op=resize&height=128`
* `http://myserver/path/to/image/img.jpg?op=resize&width=2500`
* `http://myserver/path/to/image/img.jpg?op=resize&max=1024`

And that is what *Shrimp* is about.

# Obtain and build

## Prerequisites

*Shrimp* is developed under Ubuntu (16.04/18.04) using gcc-8.1 or gcc-7.3.

*Shrimp* uses [ImageMagick](https://www.imagemagick.org) for working with images.
It is the only dependency that must be installed manually.

An sample of how to obtain and build ImageMagick-7.0.7.7:

```bash
# Install necessary packages:
apt-get install autoconf pkg-config libjpeg-dev libpng-dev libgif-dev
wget https://github.com/ImageMagick/ImageMagick/archive/7.0.7.7.zip
unzip 7.0.7.7.zip
cd ImageMagick-7.0.7.7
./configure && make
make install

#Test:
Magick++-config --cppflags --libs
```

All other dependencies come with *Shrimp*.

## Obtain *Shrimp* sources

### Obtain from repository

To build *Shrimp* from repository you need to install
[Mxx_ru](https://sourceforge.net/projects/mxxru/) 1.6.14.5 or above:

```bash
apt-get install ruby
gem install Mxx_ru
```

*Mxx_ru* helps to download necessary dependencies and used as a build tool.

Clone repo and download dependencies:

```bash
hg clone https://bitbucket.org/sobjectizerteam/shrimp-demo
cd shrimp
mxxruexternals
```

### Obtain from archive

```bash
wget https://bitbucket.org/sobjectizerteam/shrimp-demo/downloads/<ARCHIVE>
tar xjvf <ARCHIVE>
cd <UNPACKED_DIR>
```


## Build with Mxx_ru

Once you get *Shrimp* sources and its dependencies you can build it.

```bash
# Start with repository root directory
cd dev
cp local-build.rb.example local-build.rb
# Build and run unit-tests.
ruby build.rb --mxx-cpp-release
# See shrimp.app in 'target/release' directory.
```

# License

*Shrimp* is distributed under GNU Affero GPL v.3 license (see [LICENSE](./LICENSE) and [AGPL](./agpl-3.0.txt) files).

For the license of *asio* library see COPYING file in *asio* distributive.

For the license of *nodejs/http-parser* library
see LICENSE file in *nodejs/http-parser* distributive.

For the license of *fmtlib* see LICENSE file in *fmtlib* distributive.

For the license of *SObjectizer* library see
LICENSE file in *SObjectizer* distributive.

For the license of *clara* library see LICENSE file in *clara* distributive.

For the license of *CATCH* library see LICENSE file in *CATCH* distributive.
