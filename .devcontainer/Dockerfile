FROM mcr.microsoft.com/devcontainers/javascript-node:22

RUN apt-get update \
	&& apt-get install --no-install-recommends -y \
	clang-format-15 \
	# Clean cache
	&& apt-get purge -y --auto-remove -o APT::AutoRemove::RecommendsImportant=false \
	&& apt-get clean -y && rm -rf /var/lib/apt/lists/* \
	&& mv /usr/bin/clang-format-15 /usr/bin/clang-format
