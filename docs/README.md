1. Встановити залежності:
    sudo apt install pipx

    pipx ensurepath

    pipx install sphinx

    pipx inject sphinx myst-parser furo

    pipx install sphinx-autobuild

    pipx inject sphinx-autobuild myst-parser furo


2. Для збірки документації в поточному каталозі виконати:

    make build
