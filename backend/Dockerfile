# Use lightweight official Python runtime as base image
FROM python:3.12-slim

# Set working directory inside container
WORKDIR /app

# Set environment variables
ENV PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1 \
    PORT=5000 \
    DATABASE_PATH=/app/data/sensor_database.db

# Backend is stdlib-only, so there is nothing to pip install.

# Copy application source code
COPY . .

# Create volume mount point for persistent SQLite database
VOLUME ["/app/data"]

# Expose backend REST API port
EXPOSE 5000

# Healthcheck to verify backend responsiveness and SQLite connectivity
HEALTHCHECK --interval=30s --timeout=5s --start-period=5s --retries=3 \
  CMD python -c "import urllib.request, os; p = os.environ.get('PORT', '5000'); urllib.request.urlopen(f'http://localhost:{p}/health')" || exit 1

# Launch the backend server
CMD ["python", "app.py"]

