project = 'Pixeler'
copyright = '2026, Kolodieiev'
author = 'Kolodieiev'

import os
import time

extensions = [
    'sphinx.ext.viewcode',     # Посилання на вихідний код
    'myst_parser',             # Markdown підтримка
    'sphinxcontrib.mermaid'    # Діаграми
]

highlight_options = {
    'stripnl': False,
    'ensurenl': True,
}

pygments_style = 'vs' # або 'vs', 'github-dark'

highlight_language = 'cpp'

myst_enable_extensions = [
    "colon_fence",      # ::: блоки
    "deflist",          # Definition lists
    "fieldlist",        # Field lists
    "attrs_block",      # Атрибути блоків
]

myst_fence_as_directive = ["mermaid"]

suppress_warnings = [
    'myst.header',  # Ігнорувати попередження про заголовки
]

mermaid_init_js = """
mermaid.initialize({
    startOnLoad: true,
    theme: 'dark',
    flowchart: { useMaxWidth: true, htmlLabels: true },
    stateDiagram: { useMaxWidth: true }
});
"""

# Підтримка Markdown файлів
source_suffix = {
    '.rst': 'restructuredtext',
    '.md': 'markdown',
}

templates_path = ['_templates']
exclude_patterns = []

language = 'uk'
# html_theme = 'sphinx_rtd_theme'
html_theme = 'furo'
html_static_path = ['_static']
html_css_files = [
    'custom.css',
]
html_context = {
    'css_version': str(int(time.time()))
}
