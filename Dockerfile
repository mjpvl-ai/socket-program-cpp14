# Use Ubuntu as the base image
FROM ubuntu:latest

# Install required dependencies
RUN apt update && apt install -y \
    g++ cmake make \
    libpistache-dev \
    nlohmann-json3-dev \
    libcurl4-openssl-dev \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source code
COPY serverAPI.cpp .
COPY clientAPI.cpp .

# Compile the server and client
RUN g++ serverAPI.cpp -o serverAPI -lpistache -std=c++17
RUN g++ clientAPI.cpp -o clientAPI -lpistache -lcurl -std=c++17

# Expose the API port
EXPOSE 8081

# Start the server in the background and then run the client
CMD ./serverAPI & sleep 2 && ./clientAPI
