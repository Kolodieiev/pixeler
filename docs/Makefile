SPHINXOPTS 	  ?= 
SPHINXBUILD   ?= sphinx-build
SOURCEDIR     = source
BUILDDIR      = build
PORT          ?= 8000

help:
	@$(SPHINXBUILD) -M help "$(SOURCEDIR)" $(SPHINXOPTS)$(O)

.PHONY: help Makefile clean build

clean:
	@$(SPHINXBUILD) -M clean "$(SOURCEDIR)" $(SPHINXOPTS)$(O)

build:
	@echo "Starting doc build"
	sphinx-autobuild "$(SOURCEDIR)" "$(BUILDDIR)/html" \
		--open-browser \
		--delay 2 \
		--port $(PORT) \
		--ignore "$(BUILDDIR)" \
		--ignore "$(BUILDDIR)/**/*" \

%: Makefile
	@$(SPHINXBUILD) -M$@ "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS)$(O)
