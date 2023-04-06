#!/bin/bash
echo "Composing containers ..." && \
docker-compose up -d && \
docker exec -t underpass cp /code/docker/underpass-config.yaml /root/.underpass && \
echo "Building Underpass ..." && \
docker exec -t underpass mkdir -p /usr/local/lib/underpass/ && \
docker exec -w /code -t underpass ./autogen.sh && \
docker exec -w /code -t underpass mkdir -p /code/build && \
docker exec -w /code/build -t underpass ../configure CXXFLAGS="-std=c++17 -g -O0" && \
docker exec -w /code/build -t underpass make -j2 && \
docker exec -w /code/build -t underpass make install && \
docker exec -w /code/build -t underpass make install-python && \
docker exec -t underpass ln -s /usr/bin/python3.8 /usr/bin/python 
echo "Installing Postgres ..." && \
docker exec -t underpass apt update && \
docker exec -t underpass apt -y install postgresql postgresql-contrib && \
echo "Creating database ..." && \
docker exec -t underpass psql -U underpass -c 'create database underpass';
echo "Setting up database ..." && \
docker exec -w /code/setup -t underpass psql -U underpass underpass -f underpass.sql && \
echo "Setting up config ..." && \
docker exec -t underpass cp /code/docker/underpass-config.yaml /root/.underpass && \
echo "Setting up utils ..." && \
docker exec -t underpass apt -y install curl && \
docker exec -t underpass apt -y install gunicorn && \
docker exec -t underpass apt -y install nodejs npm && \
docker exec -t underpass npm cache clean -f && \
docker exec -t underpass npm install -g n && \
docker exec -t underpass n stable && \
docker exec -t underpass npm install --global yarn && \
docker exec -w /code/js -t underpass yarn install && \
docker exec -t underpass apt -y install python3-pip && \
docker exec -w /code/python -t underpass pip install -r requirements.txt && \
echo "Starting services ..." && \
docker exec -t underpass tmux new-session -d -s replicator 'cd /code/build && ./replicator -t $(date +%Y-%m-%dT%H:%M:%S -d "1 week ago")' && \
docker exec -t underpass tmux new-session -d -s rest-api 'cd /code/python/restapi && uvicorn main:app --reload --host 0.0.0.0' && \
docker exec -t underpass tmux new-session -d -s react-cosmos 'cd /code/js && yarn cosmos' && \
echo "Done!"