# Use an official Ubuntu as a base image
FROM ubuntu:20.04

# Set non-interactive mode to avoid prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Update and install dependencies
RUN apt-get update && apt-get install -y \
    protobuf-compiler \
    libprotobuf-dev \
    g++ \
    make \
    wget \
    && rm -rf /var/lib/apt/lists/*

# Set working directory in the container
WORKDIR /app

# Copy your project files into the container
COPY . /app

# Expose the port for the server to listen
EXPOSE 8082

# Set the default command to run the server
CMD ["bash"]
