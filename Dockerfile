FROM ubuntu:22.04

ARG BUILD_JOBS=2

ENV DEBIAN_FRONTEND=noninteractive
ENV GEANT4_VERSION=11.1.2
ENV ROOTSYS=/software/root_install
ENV PATH=/software/root_install/bin:${PATH}
ENV LD_LIBRARY_PATH=/software/root_install/lib:/usr/local/lib:${LD_LIBRARY_PATH}

SHELL ["/bin/bash", "-lc"]

RUN apt-get update && apt-get install -y --no-install-recommends \
    binutils \
    ca-certificates \
    cmake \
    g++ \
    gcc \
    git \
    libexpat1-dev \
    libssl-dev \
    libxerces-c-dev \
    make \
    python3 \
    wget \
    zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /software

RUN mkdir CLHEP && cd CLHEP && \
    wget --no-check-certificate https://proj-clhep.web.cern.ch/proj-clhep/dist1/clhep-2.4.7.1.tgz && \
    tar xzf clhep-2.4.7.1.tgz && \
    mkdir build && cd build && \
    cmake ../2.4.7.1/CLHEP && \
    cmake --build . --target install -j${BUILD_JOBS}

RUN apt-get update && apt-get install -y --no-install-recommends \
    nlohmann-json3-dev \
    && rm -rf /var/lib/apt/lists/*

RUN apt-get update && apt-get install -y --no-install-recommends \
    libfreetype6-dev \
    && rm -rf /var/lib/apt/lists/*

RUN git clone --branch latest-stable --depth=1 https://github.com/root-project/root.git root_src

RUN mkdir root_build root_install && cd root_build && \
    cmake \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/software/root_install \
      -Dasimage=OFF \
      -Dbuiltin_all=ON \
      -Dbuiltin_freetype=ON \
      -Dminimal=ON \
      -Dx11=OFF \
      ../root_src && \
    cmake --build . --target install -j${BUILD_JOBS}

RUN mkdir geant4 && cd geant4 && \
    wget --no-check-certificate https://gitlab.cern.ch/geant4/geant4/-/archive/v${GEANT4_VERSION}/geant4-v${GEANT4_VERSION}.tar.gz && \
    tar xzf geant4-v${GEANT4_VERSION}.tar.gz && \
    mkdir build

RUN cd /software/geant4/build && \
    source /software/root_install/bin/thisroot.sh && \
    cmake ../geant4-v${GEANT4_VERSION} \
      -DGEANT4_INSTALL_DATA=ON \
      -DGEANT4_USE_OPENGL_X11=OFF \
      -DGEANT4_USE_QT=OFF \
      -DGEANT4_USE_ROOT=ON \
      -DGEANT4_USE_SYSTEM_CLHEP=ON \
      -DCLHEP_ROOT_DIR=/usr/local && \
    cmake --build . -j${BUILD_JOBS} && \
    cmake --install .

RUN rm -rf /software/CLHEP/build /software/geant4/build /software/root_build

WORKDIR /opt/G4LArBox
COPY . /opt/G4LArBox

RUN apt-get update && apt-get install -y --no-install-recommends \
    libgsl-dev \
    && rm -rf /var/lib/apt/lists/*

RUN rm -f extern/.keep && \
    if [ ! -d extern/marley/.git ]; then \
      git clone --depth=1 https://github.com/njlane314/marley.git extern/marley; \
    fi && \
    source /software/root_install/bin/thisroot.sh && \
    make -C extern/marley/build -j${BUILD_JOBS}

RUN mkdir -p build && \
    source /software/root_install/bin/thisroot.sh && \
    source /usr/local/bin/geant4.sh && \
    cmake -S . -B build && \
    cmake --build build -j${BUILD_JOBS}

RUN chmod +x /opt/G4LArBox/docker-entrypoint.sh

ENTRYPOINT ["/opt/G4LArBox/docker-entrypoint.sh"]
CMD ["-d", "simplebox.mac", "-g", "singlegun.mac"]
