default: format lint

.PHONY: format lint install default

install:
	poetry install

lint:
	poetry run flake8
	poetry run mypy .

format:
	poetry run isort .
	poetry run black .
