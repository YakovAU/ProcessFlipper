@echo off
pip install -e .
python -c "from PIL import Image; Image.new('RGB', (64, 64), color='red').save('icon.png')"
python main.py
