FROM fedora:latest

RUN dnf update -y && dnf install -y \
    cmake \
    make \
    gcc \
    gcc-c++ \
    python3-devel \
    pybind11-devel \
    cereal-devel \
    gtest \
    gtest-devel

WORKDIR /app

COPY . /app

EXPOSE 8888/udp

RUN mkdir build && cd build && cmake .. && make -j 4

COPY scripts/run_script.sh /run_script.sh

RUN chmod +x /run_script.sh

CMD ["/run_script.sh"]