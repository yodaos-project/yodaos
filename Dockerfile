FROM node:alpine

# Install and config Supervisor
RUN apk add python make bash git openssh

# prepare sync tool
COPY tools/repo /usr/bin/
COPY tools/repo-username /usr/bin/

# prepare yodaos codebase
RUN mkdir -p /opt/codebase/
COPY manifest.xml /opt/codebase/
COPY manifests /opt/codebase/

# Add a startup script
WORKDIR /opt/codebase/
ADD ./tools/download.sh /download.sh
RUN chmod 755 /download.sh

CMD ["/download.sh"]
