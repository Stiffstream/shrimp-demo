# What Is Shrimp?

*Shrimp* is a small HTTP-server that provides shrunk images
generated from a specified set. It also supports conversion
of an image into a different format (jpg, png, gif, webp and
heic are supported).

## Basic idea

Suppose you want your server to serve resized images over HTTP.
You receive a request and use its path to determine source image and
use request query string to determine resize parameters for it.
So you end up with the following form requests to your server:

* `http://myserver/path/to/image/img.jpg?op=resize&height=128`
* `http://myserver/path/to/image/img.jpg?op=resize&width=2500`
* `http://myserver/path/to/image/img.jpg?op=resize&max=1024`

You can also request a conversion to different format:

* `http://myserver/path/to/image/img.jpg?op=resize&height=128&target-format=png`
* `http://myserver/path/to/image/img.jpg?target-format=webp`

And that is what *Shrimp* is about.

# Obtain and build

## Prerequisites

*Shrimp* is developed under Ubuntu (16.04/18.04) using gcc-8.1 or gcc-7.3.

*Shrimp* uses [ImageMagick](https://www.imagemagick.org) for working with images.
ImageMagick and all its dependencies (like libjpeg-dev, libpng-dev, libwebp,
x265, libde265, libheif and so on) must be installed manually.

All other *Shrimp*'s dependencies come with *Shrimp*.

## Obtain *Shrimp* sources

### Obtain from repository

Just use `hg clone` command to download *Shrimp* sources.

```bash
hg clone https://bitbucket.org/sobjectizerteam/shrimp-demo
```

You can also download a specific *Shrimp*'s version from a
[Downloads](https://bitbucket.org/sobjectizerteam/shrimp-demo/downloads/?tab=tags) section.

## Building

There are two approaches for building *Shrimp*:

* by using Docker;
* by using Mxx_ru.

The simplest one is Docker-build because all necessary dependencies are
downloaded and built automatically during `docker build` command.

### Docker build

You need [Docker](https://www.docker.com/) installed.

Enter into *Shrimp*'s folder and run `docker build` command:

```bash
cd shrimp-demo
docker build -t shrimpdemo .
```

All necessary dependencies will be downloaded, configured and built inside
Docker's image.

Then you can start Docker's image:

```bash
docker run -p 8080:80 \
  --mount type=bind,source=YOUR-PATH,destination=/root/images,readonly \
  shrimpdemo
```
where `YOUR-PATH` is a path to your images.

## Build with Mxx_ru

First of all you need to download, configure and install ImageMagick and
all its dependencies. ImageMagick and all necessary image encoders/decoders
should be installed in your system before you start building *Shrimp*.
You can take a look onto `Dockerfile` to see how it can be done.

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
cd shrimp-demo
mxxruexternals
```

Once you get *Shrimp* sources and its dependencies you can build it.

```bash
# Start with repository root directory
cd dev
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
