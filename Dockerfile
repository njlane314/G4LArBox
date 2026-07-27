FROM --platform=linux/amd64 carlomt/geant4:11.4.2-bookworm AS geant4

RUN /opt/geant4/bin/geant4-config --install-datasets

FROM --platform=linux/amd64 rootproject/root:6.38.00-ubuntu25.10

SHELL ["/bin/bash", "-lc"]

ENV DEBIAN_FRONTEND=noninteractive
ENV PATH=/opt/geant4/bin:${PATH}
ENV CMAKE_PREFIX_PATH=/opt/geant4:${CMAKE_PREFIX_PATH}
ENV LD_LIBRARY_PATH=/opt/geant4/lib:${LD_LIBRARY_PATH}

COPY --from=geant4 /opt/geant4 /opt/geant4
COPY --from=geant4 /g4data /g4data

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    ca-certificates \
    cmake \
    git \
    libexpat1-dev \
    libgsl-dev \
    libxerces-c-dev \
    nlohmann-json3-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /opt/G4LArBox
COPY CMakeLists.txt .

RUN source /opt/root/bin/thisroot.sh && \
    source /opt/geant4/bin/geant4.sh && \
    cmake -S . -B build-dependencies -DG4LARBOX_DEPENDENCIES_ONLY=ON

COPY . .

RUN source /opt/root/bin/thisroot.sh && \
    source /opt/geant4/bin/geant4.sh && \
    cmake -S . -B build && \
    cmake --build build -j2

ENTRYPOINT ["/opt/G4LArBox/docker-entrypoint.sh"]
CMD ["-d", "simplebox.mac", "-g", "singlegun.mac"]
