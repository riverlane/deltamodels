FROM ubuntu:20.04
ENV BUILD_THREADS=4

# To reduce warnings, see
# https://code.visualstudio.com/docs/remote/containers-advanced
ENV DEBIAN_FRONTEND=noninteractive

ENV TZ=Europe/London
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

# Install dependencies
COPY ./environment/apt-list.txt /opt/
RUN apt-get update && \
    cat /opt/apt-list.txt | \
    sed 's/#.*$//g' | \
    sed '/^$/d' | \
    tr '\n' ' ' | \
    xargs apt-get install -y

# Links for python, pip, and python debugger
RUN ln -s /usr/bin/python3 /usr/bin/python && \
    ln -s /usr/bin/pip3 /usr/bin/pip && \
    ln -s /usr/bin/pdb3 /usr/bin/pdb

# Python packages
COPY ./environment/dev_requirements.txt /opt/
COPY ./environment/install_requirements.txt /opt/
RUN pip install --upgrade pip
RUN pip install -U pip-licenses
RUN pip install --no-cache-dir -r /opt/dev_requirements.txt
RUN pip install --no-cache-dir -r /opt/install_requirements.txt

# Everything below is to create a user with the same credentials as yours.

# Add a non-root user
# These arguments should be passed on build
ARG USERNAME
ARG USER_UID
ARG USER_GID

# Create the user
RUN (groupadd --gid $USER_GID $USERNAME; exit 0) && \
    useradd --uid $USER_UID --gid $USER_GID -m $USERNAME && \
    mkdir -p /home/$USERNAME/.vscode-server /home/$USERNAME/.vscode-server-insiders && \
    chown ${USER_UID}:${USER_GID} /home/$USERNAME/.vscode-server*

# Add sudo support to install extra software if needed
RUN apt-get install -y sudo && \
    echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME && \
    chmod 0440 /etc/sudoers.d/$USERNAME

# Clean up
RUN apt-get autoremove -y && \
    apt-get clean -y && \
    rm -rf /var/lib/apt/lists/*

# Switch to the user
USER $USERNAME

ENV DEBIAN_FRONTEND=dialog \
    PYTHONPATH=${PYTHONPATH}:/workdir \
    SYSTEMC_INCLUDE=/usr/include/sysc \
    SYSTEMC_LIBDIR=/usr/lib/x86_64-linux-gnu/ \
    VERILATOR_INCLUDE=/usr/share/verilator/include/

