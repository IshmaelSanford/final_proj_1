# US Senator Tweet Sentiment Analyzer

![Build Status](https://github.com/IshmaelSanford/final_proj_1/workflows/Build%20C%2B%2B%20Project/badge.svg)

A C++ sentiment analysis tool for analyzing tweets from US Senators. This project was developed for CS101 as a final project, demonstrating lexicon-based sentiment analysis with advanced features including negation handling, intensifiers, emoji detection, and n-gram analysis.

## 📋 Project Description

This sentiment analyzer processes a CSV file of senator tweets (`tweets.csv`) and evaluates their sentiment using:
- Positive and negative word lexicons (`positive-words.txt`, `negative-words.txt`)
- Porter Stemmer algorithm (`stemmer.h`) for word normalization
- Advanced sentiment engine with context-aware analysis

### Features

**Part I: Basic Sentiment Analysis**
- Load and parse tweet data from CSV
- Tokenize tweets and normalize words using stemming
- Count positive and negative words per tweet
- Calculate sentiment scores

**Part II: Statistical Analysis**
- Identify most positive and negative tweets
- Track senator activity (most talkative senators)
- Generate summary statistics

**Extra Credit Features**
- Negation detection (e.g., "not good" reduces positive score)
- Intensifier support (e.g., "very happy" amplifies sentiment)
- Emoji sentiment analysis 😊 😢
- N-gram context analysis
- JSON export for web visualization
- ANSI-colored terminal output

## 🏗️ Building the Project

### Using g++ (Linux/Mac/MinGW)

```bash
mkdir build
cd build
g++ -std=c++17 -Wall ../project3_student.cpp -o final_proj_1
./final_proj_1
```

### Using MSVC (Windows)

```bash
cl /EHsc /std:c++17 /W4 project3_student.cpp
project3_student.exe
```

### Using VS Code

1. Open the project folder in VS Code
2. Use the integrated terminal (PowerShell or Git Bash)
3. Run one of the build commands above

## 🔄 GitHub Actions CI

This project includes automated builds via GitHub Actions. On every push or pull request:
- The code is compiled with g++ on Ubuntu
- Build artifacts are uploaded for download
- Build status is visible in the repository badge

See [`.github/workflows/build.yml`](.github/workflows/build.yml) for the complete workflow configuration.

## 📁 Project Structure

```
final_proj_1/
├── project3_student.cpp    # Main source file (single-file implementation)
├── stemmer.h               # Porter Stemmer header
├── tweets.csv              # Input data (senator tweets)
├── positive-words.txt      # Positive sentiment lexicon
├── negative-words.txt      # Negative sentiment lexicon
├── analysis.json           # Output (generated during runtime)
├── .gitignore              # Git ignore patterns
├── .github/
│   └── workflows/
│       └── build.yml       # CI/CD configuration
└── README.md               # This file
```

## 🚀 Usage

1. Ensure all data files are in the same directory as the executable
2. Run the program:
   ```bash
   ./final_proj_1    # Linux/Mac
   project3_student.exe    # Windows
   ```
3. Follow the interactive menu prompts
4. Analysis results are displayed in the terminal and exported to `analysis.json`

## 📊 Data Files

- **tweets.csv**: Contains senator names, party affiliations, and tweet text
- **positive-words.txt**: ~2000 positive sentiment words
- **negative-words.txt**: ~4800 negative sentiment words
- **stemmer.h**: Porter Stemmer implementation for word normalization

## 🗺️ Roadmap

### Future Enhancements (v1.0.1+)

- **Modular Architecture**: Refactor single-file implementation into multiple files
  - `src/sentiment_analyzer.cpp/h` - Core sentiment engine
  - `src/tweet_parser.cpp/h` - CSV parsing and data structures
  - `src/lexicon_loader.cpp/h` - Word list management
  - `src/cli_interface.cpp/h` - User interface with ANSI colors
  - `src/json_exporter.cpp/h` - JSON output generation
  
- **Enhanced CLI**: Improved ANSI-colored terminal interface with better formatting

- **Advanced Metrics**: 
  - Sentiment trend analysis over time
  - Comparison between political parties
  - Topic-based sentiment clustering

- **Web Visualization**: React-based dashboard consuming `analysis.json`
  - Interactive charts and graphs
  - Tweet timeline visualization
  - Senator comparison tools

## 📜 Version History

### v1.0.0-beta (Current)
- Initial single-file C++ implementation
- Complete Part I and Part II functionality
- Extra credit features: negation, intensifiers, emojis, n-grams
- JSON export capability
- ANSI-colored terminal output
- GitHub Actions CI integration
- Comprehensive documentation

### Future: v1.0.1
- Planned: Modular multi-file architecture
- Planned: Enhanced CLI with better ANSI formatting
- Planned: Additional sentiment metrics

## 📝 License

This project is for educational purposes as part of CS101 coursework.

## 👤 Author

Ishmael Sanford

## 🙏 Acknowledgments

- CS101 course staff for project specifications
- Porter Stemmer algorithm by Martin Porter
- Sentiment lexicons from various NLP research sources
