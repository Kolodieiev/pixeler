project = 'Pixeler'
copyright = '2026, Kolodieiev'
author = 'Kolodieiev'

import os

extensions = [
    'sphinx.ext.viewcode',     # Посилання на вихідний код
    'myst_parser',             # Markdown підтримка
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

suppress_warnings = [
    'myst.header',  # Ігнорувати попередження про заголовки
]

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
