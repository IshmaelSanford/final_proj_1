/*
 * CS101 Final Project - Senator Tweet Sentiment Analysis
 * 
 * This program analyzes senator tweets for sentiment using:
 * - Part I: Basic lexicon-based sentiment counting
 * - Part II: Most positive/negative tweets and talkative senators
 * - Extra Credit: Advanced sentiment engine with negation, intensifiers, emojis, n-grams
 * - JSON export for React app integration
 */

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <ctime>
#include "stemmer.h"

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

using namespace std;

// ============================================================================
// ANSI COLOR SYSTEM
// ============================================================================

namespace Color {
    // ANSI escape codes
    const string RESET = "\033[0m";
    const string BOLD = "\033[1m";
    const string DIM = "\033[2m";
    const string ITALIC = "\033[3m";
    const string UNDERLINE = "\033[4m";
    
    // Foreground colors
    const string BLACK = "\033[30m";
    const string RED = "\033[31m";
    const string GREEN = "\033[32m";
    const string YELLOW = "\033[33m";
    const string BLUE = "\033[34m";
    const string MAGENTA = "\033[35m";
    const string CYAN = "\033[36m";
    const string WHITE = "\033[37m";
    
    // Bright foreground colors
    const string BRIGHT_BLACK = "\033[90m";
    const string BRIGHT_RED = "\033[91m";
    const string BRIGHT_GREEN = "\033[92m";
    const string BRIGHT_YELLOW = "\033[93m";
    const string BRIGHT_BLUE = "\033[94m";
    const string BRIGHT_MAGENTA = "\033[95m";
    const string BRIGHT_CYAN = "\033[96m";
    const string BRIGHT_WHITE = "\033[97m";
    
    // Background colors
    const string BG_BLACK = "\033[40m";
    const string BG_RED = "\033[41m";
    const string BG_GREEN = "\033[42m";
    const string BG_YELLOW = "\033[43m";
    const string BG_BLUE = "\033[44m";
    const string BG_MAGENTA = "\033[45m";
    const string BG_CYAN = "\033[46m";
    const string BG_WHITE = "\033[47m";
    
    // Semantic colors for sentiment analysis
    const string POSITIVE = GREEN;
    const string NEGATIVE = RED;
    const string NEUTRAL = YELLOW;
    const string INFO = CYAN;
    const string WARNING = YELLOW;
    const string ERROR_COLOR = RED;
    const string SUCCESS = GREEN;
    const string HEADER = BOLD + MAGENTA;
    const string SUBHEADER = BOLD + CYAN;
    const string MENU_SELECTED = BG_WHITE + BLACK;
    const string MENU_NORMAL = WHITE;
    const string STAT_LABEL = BLUE;
    const string STAT_VALUE = WHITE;
    
    // Enable ANSI colors on Windows
    void enableColors() {
        #ifdef _WIN32
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD dwMode = 0;
        GetConsoleMode(hOut, &dwMode);
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
        #endif
    }
    
    // Helper function to colorize text
    string colorize(const string& text, const string& color) {
        return color + text + RESET;
    }
    
    // Helper function to get color based on percentage value (red -> yellow -> green)
    string getPercentColor(double percent) {
        if (percent < 3.0) return RED;
        else if (percent < 5.0) return YELLOW;
        else return GREEN;
    }
}

// ============================================================================
// ARROW-KEY MENU SYSTEM
// ============================================================================

class ArrowMenu {
private:
    vector<string> options;
    string title;
    int selectedIndex;
    
    // Get a single keypress (cross-platform)
    int getKey() {
        #ifdef _WIN32
        int ch = _getch();
        if (ch == 0 || ch == 224) { // Arrow keys return two codes on Windows
            ch = _getch();
            switch (ch) {
                case 72: return 'A'; // Up arrow
                case 80: return 'B'; // Down arrow
                case 75: return 'D'; // Left arrow
                case 77: return 'C'; // Right arrow
            }
        }
        return ch;
        #else
        struct termios oldt, newt;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        
        int ch = getchar();
        if (ch == 27) { // ESC sequence
            getchar(); // Skip '['
            ch = getchar();
        }
        
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        return ch;
        #endif
    }
    
    void clearScreen() {
        #ifdef _WIN32
        system("cls");
        #else
        system("clear");
        #endif
    }
    
    void displayMenu() {
        clearScreen();
        
        // Display title
        cout << "\n  " << title << "\n";
        cout << "  " << string(title.length(), '=') << "\n\n";
        
        // Display options
        for (size_t i = 0; i < options.size(); i++) {
            if (i == selectedIndex) {
                cout << Color::MENU_SELECTED << "  " << options[i] << "  " << Color::RESET << "\n";
            } else {
                cout << "  " << options[i] << "\n";
            }
        }
        
        cout << "\n  Use Up/Down arrows to navigate, Enter to select, Q to quit\n";
    }
    
public:
    ArrowMenu(const string& menuTitle, const vector<string>& menuOptions) 
        : title(menuTitle), options(menuOptions), selectedIndex(0) {}
    
    // Returns the selected option index, or -1 if quit
    int show() {
        while (true) {
            displayMenu();
            
            int key = getKey();
            
            switch (key) {
                case 'A': // Up arrow
                case 'w':
                case 'W':
                    if (selectedIndex > 0) selectedIndex--;
                    break;
                    
                case 'B': // Down arrow
                case 's':
                case 'S':
                    if (selectedIndex < options.size() - 1) selectedIndex++;
                    break;
                    
                case 13: // Enter
                case 10: // Enter (Unix)
                    return selectedIndex;
                    
                case 'q':
                case 'Q':
                case 27: // ESC
                    return -1;
            }
        }
    }
    
    // Static helper to create and show a menu in one call
    static int showMenu(const string& title, const vector<string>& options) {
        ArrowMenu menu(title, options);
        return menu.show();
    }
};

// ============================================================================
// DATA STRUCTURES
// ============================================================================

struct Tweet {
    string tweetId;
    string userId;
    string datetime;
    string senatorName;
    string text;
};

struct SenatorStats {
    string name;
    int totalTweets = 0;
    int totalWords = 0;
    int totalPositiveWords = 0;
    int totalNegativeWords = 0;
    double positivePercent = 0.0;
    double negativePercent = 0.0;
};

struct TweetSentiment {
    const Tweet* tweet = nullptr;
    int positiveCount = 0;
    int negativeCount = 0;
    int totalWords = 0;
    int rawScore = 0; // positiveCount - negativeCount
};

struct TalkStats {
    string name;
    int tweetCount = 0;
    int totalWords = 0;
    double avgWordsPerTweet = 0.0;
};

struct AdvancedTweetAnalysis {
    string tweetId;
    string senatorName;
    string datetime;
    string text;

    int totalWords = 0;
    int posWordCount = 0;
    int negWordCount = 0;
    int neutralWordCount = 0;

    double baseSentimentScore = 0.0;
    double adjustedSentimentScore = 0.0;

    int negationHits = 0;
    int intensifierHits = 0;
    int downtonerHits = 0;

    int exclamationCount = 0;
    int questionCount = 0;
    int allCapsWordCount = 0;

    int emojiPositiveCount = 0;
    int emojiNegativeCount = 0;
    int slangPositiveCount = 0;
    int slangNegativeCount = 0;

    int ngramPositiveHits = 0;
    int ngramNegativeHits = 0;
};

struct AdvancedSenatorSummary {
    string name;
    int tweetCount = 0;
    double avgBaseSentiment = 0.0;
    double avgAdjustedSentiment = 0.0;
    double avgPosPercent = 0.0;
    double avgNegPercent = 0.0;
    double avgAllCaps = 0.0;
    double avgExclamations = 0.0;
    double avgStyleScore = 0.0;
    AdvancedTweetAnalysis mostPositiveTweet;
    AdvancedTweetAnalysis mostNegativeTweet;
};

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

// File I/O
vector<vector<string>> read_tweets_csv_file();
vector<string> readEmotionFile(string path);

// Data conversion and extraction
vector<Tweet> convertToTweets(const vector<vector<string>>& rawData);
vector<string> getUniqueSenators(const vector<Tweet>& tweets);

// Word processing
string normalizeWord(const string& raw);
bool isAllCaps(const string& word);
string escapeJsonString(const string& s);

// Part I: Base sentiment analysis
unordered_set<string> buildLexiconSet(const vector<string>& words);
vector<SenatorStats> computeBaseSenatorStats(
    const vector<Tweet>& tweets,
    const unordered_set<string>& positiveLexicon,
    const unordered_set<string>& negativeLexicon
);
void printBaseSentimentTable(const vector<SenatorStats>& stats);

// Part II Capability 1: Most positive/negative tweets
TweetSentiment analyzeTweetLexiconOnly(
    const Tweet& tweet,
    const unordered_set<string>& positiveLexicon,
    const unordered_set<string>& negativeLexicon
);
void showMostPositiveAndNegativeTweetForSenator(
    const vector<Tweet>& tweets,
    const unordered_set<string>& positiveLexicon,
    const unordered_set<string>& negativeLexicon,
    const string& senatorName
);

// Part II Capability 2: Talkative senators
vector<TalkStats> computeTalkStats(const vector<Tweet>& tweets);
void printTalkStatsAndMostTalkative(const vector<TalkStats>& stats);

// Extra Credit: Advanced sentiment engine
unordered_map<string, double> buildWordPolarityMap(
    const vector<string>& posWords,
    const vector<string>& negWords
);
unordered_set<string> buildNegationWords();
unordered_set<string> buildIntensifiers();
unordered_set<string> buildDowntoners();
unordered_map<string, double> buildNgramPolarity();
unordered_set<string> buildPositiveEmojisSlang();
unordered_set<string> buildNegativeEmojisSlang();

AdvancedTweetAnalysis analyzeTweetAdvanced(
    const Tweet& tweet,
    const unordered_map<string, double>& wordPolarity,
    const unordered_set<string>& negationWords,
    const unordered_set<string>& intensifiers,
    const unordered_set<string>& downtoners,
    const unordered_map<string, double>& ngramPolarity,
    const unordered_set<string>& positiveEmojisSlang,
    const unordered_set<string>& negativeEmojisSlang
);

vector<AdvancedTweetAnalysis> analyzeAllTweetsAdvanced(
    const vector<Tweet>& tweets,
    const unordered_map<string, double>& wordPolarity,
    const unordered_set<string>& negationWords,
    const unordered_set<string>& intensifiers,
    const unordered_set<string>& downtoners,
    const unordered_map<string, double>& ngramPolarity,
    const unordered_set<string>& positiveEmojisSlang,
    const unordered_set<string>& negativeEmojisSlang
);

vector<AdvancedSenatorSummary> summarizeAdvancedBySenator(
    const vector<AdvancedTweetAnalysis>& perTweet
);

// JSON export
void writeAnalysisJson(
    const vector<SenatorStats>& baseStats,
    const vector<AdvancedSenatorSummary>& advancedSummaries,
    const vector<AdvancedTweetAnalysis>& allTweetAnalyses,
    const string& filename
);

// ============================================================================
// FILE I/O IMPLEMENTATIONS
// ============================================================================

vector<vector<string>> read_tweets_csv_file()
{
    vector<vector<string>> tweets;
    fstream fin;
    fin.open("tweets.csv", ios::in);
    if (!fin.is_open()) {
        cerr << "Error: Could not open tweets.csv" << endl;
        return tweets;
    }
    
    string line;
    vector<string> row;
    getline(fin, line); // Skip header
    
    while (getline(fin, line))
    {
        row.clear();
        stringstream s(line);
        string word;
        while (getline(s, word, '|')) {
            row.push_back(word);
        }
        if(row.size() == 5)
            tweets.push_back(row);
    }
    
    fin.close();
    return tweets;
}

vector<string> readEmotionFile(string path)
{
    vector<string> emotionWords;
    fstream newfile;
    newfile.open(path, ios::in);
    
    if(newfile.is_open()) {
        string line;
        while(getline(newfile, line)) {
            if (!line.empty())
                emotionWords.push_back(line);
        }
        newfile.close();
    }
    return emotionWords;
}

// ============================================================================
// DATA CONVERSION
// ============================================================================

vector<Tweet> convertToTweets(const vector<vector<string>>& rawData)
{
    vector<Tweet> tweets;
    for (const auto& row : rawData) {
        if (row.size() == 5) {
            Tweet t;
            t.tweetId = row[0];
            t.userId = row[1];
            t.datetime = row[2];
            t.senatorName = row[3];
            t.text = row[4];
            tweets.push_back(t);
        }
    }
    return tweets;
}

vector<string> getUniqueSenators(const vector<Tweet>& tweets)
{
    unordered_set<string> senatorSet;
    for (const auto& tweet : tweets) {
        senatorSet.insert(tweet.senatorName);
    }
    return vector<string>(senatorSet.begin(), senatorSet.end());
}

// ============================================================================
// WORD PROCESSING
// ============================================================================

string normalizeWord(const string& raw)
{
    if (raw.empty()) return "";
    
    // Find first and last alphanumeric character
    int start = 0, end = raw.length() - 1;
    
    while (start <= end && !isalnum(raw[start])) start++;
    while (end >= start && !isalnum(raw[end])) end--;
    
    if (start > end) return "";
    
    // Extract word and convert to lowercase
    string word = raw.substr(start, end - start + 1);
    transform(word.begin(), word.end(), word.begin(), ::tolower);
    
    // Stem the word
    return stemString(word);
}

bool isAllCaps(const string& word)
{
    if (word.length() <= 1) return false;
    
    int alphaCount = 0;
    for (char c : word) {
        if (isalpha(c)) {
            alphaCount++;
            if (!isupper(c)) return false;
        }
    }
    return alphaCount > 0;
}

string escapeJsonString(const string& s)
{
    string result;
    for (char c : s) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c;
        }
    }
    return result;
}

// ============================================================================
// PART I: BASE SENTIMENT ANALYSIS
// ============================================================================

unordered_set<string> buildLexiconSet(const vector<string>& words)
{
    unordered_set<string> lexicon;
    for (const auto& word : words) {
        string stemmed = stemString(word);
        if (!stemmed.empty())
            lexicon.insert(stemmed);
    }
    return lexicon;
}

vector<SenatorStats> computeBaseSenatorStats(
    const vector<Tweet>& tweets,
    const unordered_set<string>& positiveLexicon,
    const unordered_set<string>& negativeLexicon)
{
    unordered_map<string, SenatorStats> statsMap;
    
    for (const auto& tweet : tweets) {
        auto& stats = statsMap[tweet.senatorName];
        stats.name = tweet.senatorName;
        stats.totalTweets++;
        
        stringstream ss(tweet.text);
        string rawWord;
        while (ss >> rawWord) {
            string normalized = normalizeWord(rawWord);
            if (normalized.empty()) continue;
            
            stats.totalWords++;
            
            if (positiveLexicon.count(normalized)) {
                stats.totalPositiveWords++;
            }
            if (negativeLexicon.count(normalized)) {
                stats.totalNegativeWords++;
            }
        }
    }
    
    vector<SenatorStats> result;
    for (auto& pair : statsMap) {
        auto& stats = pair.second;
        if (stats.totalWords > 0) {
            stats.positivePercent = 100.0 * stats.totalPositiveWords / stats.totalWords;
            stats.negativePercent = 100.0 * stats.totalNegativeWords / stats.totalWords;
        }
        result.push_back(stats);
    }
    
    return result;
}

void printBaseSentimentTable(const vector<SenatorStats>& stats)
{
    cout << "\n========================================" << endl;
    cout << "PART I: BASE SENTIMENT ANALYSIS" << endl;
    cout << "========================================" << endl;
    cout << left << setw(30) << "Senator" 
         << right << setw(12) << "Positive %" 
         << setw(12) << "Negative %" << endl;
    cout << string(54, '-') << endl;
    
    for (const auto& s : stats) {
        cout << left << setw(30) << s.name;
        
        // Color code positive percent
        cout << Color::getPercentColor(s.positivePercent) 
             << right << setw(12) << fixed << setprecision(5) << s.positivePercent 
             << Color::RESET;
        
        // Color code negative percent
        cout << Color::getPercentColor(s.negativePercent) 
             << setw(12) << fixed << setprecision(5) << s.negativePercent 
             << Color::RESET << endl;
    }
    cout << endl;
}

// ============================================================================
// PART II CAPABILITY 1: MOST POSITIVE/NEGATIVE TWEETS
// ============================================================================

TweetSentiment analyzeTweetLexiconOnly(
    const Tweet& tweet,
    const unordered_set<string>& positiveLexicon,
    const unordered_set<string>& negativeLexicon)
{
    TweetSentiment result;
    result.tweet = &tweet;
    
    stringstream ss(tweet.text);
    string rawWord;
    while (ss >> rawWord) {
        string normalized = normalizeWord(rawWord);
        if (normalized.empty()) continue;
        
        result.totalWords++;
        
        if (positiveLexicon.count(normalized)) {
            result.positiveCount++;
        }
        if (negativeLexicon.count(normalized)) {
            result.negativeCount++;
        }
    }
    
    result.rawScore = result.positiveCount - result.negativeCount;
    return result;
}

void showMostPositiveAndNegativeTweetForSenator(
    const vector<Tweet>& tweets,
    const unordered_set<string>& positiveLexicon,
    const unordered_set<string>& negativeLexicon,
    const string& senatorName)
{
    TweetSentiment mostPositive, mostNegative;
    mostPositive.rawScore = INT_MIN;
    mostNegative.rawScore = INT_MAX;
    
    bool found = false;
    for (const auto& tweet : tweets) {
        if (tweet.senatorName == senatorName) {
            found = true;
            TweetSentiment sentiment = analyzeTweetLexiconOnly(tweet, positiveLexicon, negativeLexicon);
            
            if (sentiment.rawScore > mostPositive.rawScore) {
                mostPositive = sentiment;
            }
            if (sentiment.rawScore < mostNegative.rawScore) {
                mostNegative = sentiment;
            }
        }
    }
    
    if (!found) {
        cout << "Senator not found: " << senatorName << endl;
        return;
    }
    
    cout << "\n========================================" << endl;
    cout << "MOST POSITIVE/NEGATIVE TWEETS FOR: " << senatorName << endl;
    cout << "========================================" << endl;
    
    cout << "\nMOST POSITIVE TWEET:" << endl;
    cout << "Text: " << mostPositive.tweet->text << endl;
    cout << "Positive words: " << mostPositive.positiveCount << endl;
    cout << "Negative words: " << mostPositive.negativeCount << endl;
    cout << "Total words: " << mostPositive.totalWords << endl;
    cout << "Raw score: " << mostPositive.rawScore << endl;
    
    cout << "\nMOST NEGATIVE TWEET:" << endl;
    cout << "Text: " << mostNegative.tweet->text << endl;
    cout << "Positive words: " << mostNegative.positiveCount << endl;
    cout << "Negative words: " << mostNegative.negativeCount << endl;
    cout << "Total words: " << mostNegative.totalWords << endl;
    cout << "Raw score: " << mostNegative.rawScore << endl;
    cout << endl;
}

// ============================================================================
// PART II CAPABILITY 2: TALKATIVE SENATORS
// ============================================================================

vector<TalkStats> computeTalkStats(const vector<Tweet>& tweets)
{
    unordered_map<string, TalkStats> statsMap;
    
    for (const auto& tweet : tweets) {
        auto& stats = statsMap[tweet.senatorName];
        stats.name = tweet.senatorName;
        stats.tweetCount++;
        
        stringstream ss(tweet.text);
        string rawWord;
        while (ss >> rawWord) {
            string normalized = normalizeWord(rawWord);
            if (!normalized.empty()) {
                stats.totalWords++;
            }
        }
    }
    
    vector<TalkStats> result;
    for (auto& pair : statsMap) {
        auto& stats = pair.second;
        if (stats.tweetCount > 0) {
            stats.avgWordsPerTweet = (double)stats.totalWords / stats.tweetCount;
        }
        result.push_back(stats);
    }
    
    return result;
}

void printTalkStatsAndMostTalkative(const vector<TalkStats>& stats)
{
    cout << "\n========================================" << endl;
    cout << "PART II: TALKATIVE SENATORS" << endl;
    cout << "========================================" << endl;
    cout << left << setw(30) << "Senator"
         << right << setw(12) << "Tweet Count"
         << setw(18) << "Avg Words/Tweet" << endl;
    cout << string(60, '-') << endl;
    
    TalkStats mostTweets = stats[0];
    TalkStats mostWordy = stats[0];
    
    for (const auto& s : stats) {
        cout << left << setw(30) << s.name
             << right << setw(12) << s.tweetCount
             << setw(18) << fixed << setprecision(2) << s.avgWordsPerTweet << endl;
        
        if (s.tweetCount > mostTweets.tweetCount) {
            mostTweets = s;
        }
        if (s.avgWordsPerTweet > mostWordy.avgWordsPerTweet) {
            mostWordy = s;
        }
    }
    
    cout << "\nMOST TWEETS: " << mostTweets.name 
         << " (" << mostTweets.tweetCount << " tweets)" << endl;
    cout << "HIGHEST AVG WORDS/TWEET: " << mostWordy.name 
         << " (" << fixed << setprecision(2) << mostWordy.avgWordsPerTweet << " words)" << endl;
    cout << endl;
}

// ============================================================================
// EXTRA CREDIT: ADVANCED SENTIMENT ENGINE - LEXICON BUILDING
// ============================================================================

unordered_map<string, double> buildWordPolarityMap(
    const vector<string>& posWords,
    const vector<string>& negWords)
{
    unordered_map<string, double> polarityMap;
    
    // Add positive words with default weight +1.0
    for (const auto& word : posWords) {
        string stemmed = stemString(word);
        if (!stemmed.empty()) {
            polarityMap[stemmed] = 1.0;
        }
    }
    
    // Add negative words with default weight -1.0
    for (const auto& word : negWords) {
        string stemmed = stemString(word);
        if (!stemmed.empty()) {
            polarityMap[stemmed] = -1.0;
        }
    }
    
    // Override specific strong words
    polarityMap[stemString("love")] = 2.0;
    polarityMap[stemString("amazing")] = 2.0;
    polarityMap[stemString("excellent")] = 2.0;
    polarityMap[stemString("hate")] = -2.0;
    polarityMap[stemString("terrible")] = -2.0;
    polarityMap[stemString("horrible")] = -2.0;
    
    return polarityMap;
}

unordered_set<string> buildNegationWords()
{
    return {
        stemString("not"), stemString("no"), stemString("never"),
        stemString("none"), stemString("nobody"), stemString("nothing"),
        stemString("neither"), stemString("nowhere"), stemString("hardly"),
        stemString("barely"), stemString("scarcely"), "n't"
    };
}

unordered_set<string> buildIntensifiers()
{
    return {
        stemString("very"), stemString("really"), stemString("extremely"),
        stemString("so"), stemString("super"), stemString("highly"),
        stemString("absolutely"), stemString("completely"), stemString("totally")
    };
}

unordered_set<string> buildDowntoners()
{
    return {
        stemString("slightly"), stemString("somewhat"), stemString("kind"),
        stemString("bit"), stemString("little"), stemString("fairly"),
        stemString("rather"), stemString("quite")
    };
}

unordered_map<string, double> buildNgramPolarity()
{
    unordered_map<string, double> ngramMap;
    
    // Positive phrases
    ngramMap["so much fun"] = 2.0;
    ngramMap["great job"] = 1.5;
    ngramMap["well done"] = 1.5;
    ngramMap["thank you"] = 1.0;
    ngramMap["looking forward"] = 1.5;
    
    // Negative phrases
    ngramMap["sick of"] = -2.0;
    ngramMap["waste of time"] = -2.0;
    ngramMap["so tired of"] = -1.5;
    ngramMap["fed up"] = -1.5;
    ngramMap["not good"] = -1.5;
    
    return ngramMap;
}

unordered_set<string> buildPositiveEmojisSlang()
{
    return {
        "lol", "lmao", "haha", "hehe", "yay", "awesome",
        "😂", "🤣", "😊", "😃", "😄", "❤️", "💙", "👍", "✨"
    };
}

unordered_set<string> buildNegativeEmojisSlang()
{
    return {
        "ugh", "omg", "wtf", "smh",
        "💀", "😡", "😭", "😢", "👎", "😠"
    };
}

// ============================================================================
// EXTRA CREDIT: ADVANCED SENTIMENT ENGINE - ANALYSIS
// ============================================================================

AdvancedTweetAnalysis analyzeTweetAdvanced(
    const Tweet& tweet,
    const unordered_map<string, double>& wordPolarity,
    const unordered_set<string>& negationWords,
    const unordered_set<string>& intensifiers,
    const unordered_set<string>& downtoners,
    const unordered_map<string, double>& ngramPolarity,
    const unordered_set<string>& positiveEmojisSlang,
    const unordered_set<string>& negativeEmojisSlang)
{
    AdvancedTweetAnalysis analysis;
    analysis.tweetId = tweet.tweetId;
    analysis.senatorName = tweet.senatorName;
    analysis.datetime = tweet.datetime;
    analysis.text = tweet.text;
    
    // Count punctuation
    for (char c : tweet.text) {
        if (c == '!') analysis.exclamationCount++;
        if (c == '?') analysis.questionCount++;
    }
    
    // Tokenize
    stringstream ss(tweet.text);
    string rawWord;
    vector<string> rawTokens;
    vector<string> normalizedTokens;
    
    while (ss >> rawWord) {
        rawTokens.push_back(rawWord);
        string normalized = normalizeWord(rawWord);
        normalizedTokens.push_back(normalized);
        
        // Check for ALL CAPS
        if (isAllCaps(rawWord)) {
            analysis.allCapsWordCount++;
        }
    }
    
    // Analyze tokens
    for (size_t i = 0; i < normalizedTokens.size(); i++) {
        const string& token = normalizedTokens[i];
        const string& rawToken = rawTokens[i];
        
        if (token.empty()) continue;
        analysis.totalWords++;
        
        // Check for emojis/slang in raw token (lowercase)
        string lowerRaw = rawToken;
        transform(lowerRaw.begin(), lowerRaw.end(), lowerRaw.begin(), ::tolower);
        
        if (positiveEmojisSlang.count(lowerRaw)) {
            analysis.emojiPositiveCount++;
            analysis.adjustedSentimentScore += 1.0;
            continue;
        }
        if (negativeEmojisSlang.count(lowerRaw)) {
            analysis.emojiNegativeCount++;
            analysis.adjustedSentimentScore -= 1.0;
            continue;
        }
        
        // Check for n-grams (bigrams and trigrams)
        bool inNgram = false;
        if (i + 2 < normalizedTokens.size()) {
            string trigram = normalizedTokens[i] + " " + normalizedTokens[i+1] + " " + normalizedTokens[i+2];
            if (ngramPolarity.count(trigram)) {
                double score = ngramPolarity.at(trigram);
                analysis.adjustedSentimentScore += score;
                if (score > 0) analysis.ngramPositiveHits++;
                else analysis.ngramNegativeHits++;
                inNgram = true;
            }
        }
        if (!inNgram && i + 1 < normalizedTokens.size()) {
            string bigram = normalizedTokens[i] + " " + normalizedTokens[i+1];
            if (ngramPolarity.count(bigram)) {
                double score = ngramPolarity.at(bigram);
                analysis.adjustedSentimentScore += score;
                if (score > 0) analysis.ngramPositiveHits++;
                else analysis.ngramNegativeHits++;
                inNgram = true;
            }
        }
        
        if (inNgram) continue;
        
        // Check for negation/intensifier/downtoner markers
        if (negationWords.count(token)) {
            analysis.negationHits++;
        }
        if (intensifiers.count(token)) {
            analysis.intensifierHits++;
        }
        if (downtoners.count(token)) {
            analysis.downtonerHits++;
        }
        
        // Analyze sentiment word
        if (wordPolarity.count(token)) {
            double baseWeight = wordPolarity.at(token);
            double adjustedWeight = baseWeight;
            
            // Check context (look back 1-2 tokens)
            bool hasNegation = false;
            bool hasIntensifier = false;
            bool hasDowntoner = false;
            
            for (int j = 1; j <= 2 && (int)i - j >= 0; j++) {
                const string& prevToken = normalizedTokens[i - j];
                if (negationWords.count(prevToken)) hasNegation = true;
                if (intensifiers.count(prevToken)) hasIntensifier = true;
                if (downtoners.count(prevToken)) hasDowntoner = true;
            }
            
            // Apply modifiers
            if (hasNegation) {
                adjustedWeight *= -0.7;
            }
            if (hasIntensifier) {
                adjustedWeight *= 1.5;
            }
            if (hasDowntoner) {
                adjustedWeight *= 0.5;
            }
            
            analysis.baseSentimentScore += baseWeight;
            analysis.adjustedSentimentScore += adjustedWeight;
            
            if (baseWeight > 0) analysis.posWordCount++;
            else if (baseWeight < 0) analysis.negWordCount++;
        } else {
            analysis.neutralWordCount++;
        }
    }
    
    // Adjust for punctuation emphasis
    if (analysis.exclamationCount > 0) {
        analysis.adjustedSentimentScore *= (1.0 + 0.05 * analysis.exclamationCount);
    }
    
    return analysis;
}

vector<AdvancedTweetAnalysis> analyzeAllTweetsAdvanced(
    const vector<Tweet>& tweets,
    const unordered_map<string, double>& wordPolarity,
    const unordered_set<string>& negationWords,
    const unordered_set<string>& intensifiers,
    const unordered_set<string>& downtoners,
    const unordered_map<string, double>& ngramPolarity,
    const unordered_set<string>& positiveEmojisSlang,
    const unordered_set<string>& negativeEmojisSlang)
{
    vector<AdvancedTweetAnalysis> results;
    
    for (const auto& tweet : tweets) {
        results.push_back(analyzeTweetAdvanced(
            tweet, wordPolarity, negationWords, intensifiers, downtoners,
            ngramPolarity, positiveEmojisSlang, negativeEmojisSlang
        ));
    }
    
    return results;
}

vector<AdvancedSenatorSummary> summarizeAdvancedBySenator(
    const vector<AdvancedTweetAnalysis>& perTweet)
{
    unordered_map<string, vector<AdvancedTweetAnalysis>> bySenator;
    
    for (const auto& analysis : perTweet) {
        bySenator[analysis.senatorName].push_back(analysis);
    }
    
    vector<AdvancedSenatorSummary> summaries;
    
    for (auto& pair : bySenator) {
        AdvancedSenatorSummary summary;
        summary.name = pair.first;
        summary.tweetCount = pair.second.size();
        
        double totalBase = 0.0, totalAdjusted = 0.0;
        double totalPosPercent = 0.0, totalNegPercent = 0.0;
        double totalAllCaps = 0.0, totalExclamations = 0.0;
        
        double maxAdjusted = -1e9, minAdjusted = 1e9;
        
        for (const auto& analysis : pair.second) {
            totalBase += analysis.baseSentimentScore;
            totalAdjusted += analysis.adjustedSentimentScore;
            
            if (analysis.totalWords > 0) {
                totalPosPercent += 100.0 * analysis.posWordCount / analysis.totalWords;
                totalNegPercent += 100.0 * analysis.negWordCount / analysis.totalWords;
            }
            
            totalAllCaps += analysis.allCapsWordCount;
            totalExclamations += analysis.exclamationCount;
            
            if (analysis.adjustedSentimentScore > maxAdjusted) {
                maxAdjusted = analysis.adjustedSentimentScore;
                summary.mostPositiveTweet = analysis;
            }
            if (analysis.adjustedSentimentScore < minAdjusted) {
                minAdjusted = analysis.adjustedSentimentScore;
                summary.mostNegativeTweet = analysis;
            }
        }
        
        int count = summary.tweetCount;
        summary.avgBaseSentiment = totalBase / count;
        summary.avgAdjustedSentiment = totalAdjusted / count;
        summary.avgPosPercent = totalPosPercent / count;
        summary.avgNegPercent = totalNegPercent / count;
        summary.avgAllCaps = totalAllCaps / count;
        summary.avgExclamations = totalExclamations / count;
        
        // Simple style score combining punctuation and casing
        summary.avgStyleScore = summary.avgExclamations * 2.0 + summary.avgAllCaps * 1.5;
        
        summaries.push_back(summary);
    }
    
    return summaries;
}

// ============================================================================
// JSON EXPORT
// ============================================================================

void writeAnalysisJson(
    const vector<SenatorStats>& baseStats,
    const vector<AdvancedSenatorSummary>& advancedSummaries,
    const vector<AdvancedTweetAnalysis>& allTweetAnalyses,
    const string& filename)
{
    ofstream out(filename);
    if (!out.is_open()) {
        cerr << "Error: Could not create " << filename << endl;
        return;
    }
    
    // Get current timestamp
    time_t now = time(0);
    char timestamp[100];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));
    
    out << "{\n";
    out << "  \"generatedAt\": \"" << timestamp << "\",\n";
    out << "  \"senators\": [\n";
    
    // Build maps for easy lookup
    unordered_map<string, SenatorStats> baseMap;
    for (const auto& stat : baseStats) {
        baseMap[stat.name] = stat;
    }
    
    unordered_map<string, vector<AdvancedTweetAnalysis>> tweetsBySenator;
    for (const auto& analysis : allTweetAnalyses) {
        tweetsBySenator[analysis.senatorName].push_back(analysis);
    }
    
    for (size_t i = 0; i < advancedSummaries.size(); i++) {
        const auto& summary = advancedSummaries[i];
        const auto& base = baseMap[summary.name];
        
        out << "    {\n";
        out << "      \"name\": \"" << escapeJsonString(summary.name) << "\",\n";
        
        // Base stats
        out << "      \"baseStats\": {\n";
        out << "        \"totalTweets\": " << base.totalTweets << ",\n";
        out << "        \"totalWords\": " << base.totalWords << ",\n";
        out << "        \"positivePercent\": " << fixed << setprecision(2) << base.positivePercent << ",\n";
        out << "        \"negativePercent\": " << fixed << setprecision(2) << base.negativePercent << "\n";
        out << "      },\n";
        
        // Advanced summary
        out << "      \"advancedSummary\": {\n";
        out << "        \"avgBaseSentiment\": " << fixed << setprecision(2) << summary.avgBaseSentiment << ",\n";
        out << "        \"avgAdjustedSentiment\": " << fixed << setprecision(2) << summary.avgAdjustedSentiment << ",\n";
        out << "        \"avgPosPercent\": " << fixed << setprecision(2) << summary.avgPosPercent << ",\n";
        out << "        \"avgNegPercent\": " << fixed << setprecision(2) << summary.avgNegPercent << ",\n";
        out << "        \"avgAllCaps\": " << fixed << setprecision(2) << summary.avgAllCaps << ",\n";
        out << "        \"avgExclamations\": " << fixed << setprecision(2) << summary.avgExclamations << ",\n";
        out << "        \"avgStyleScore\": " << fixed << setprecision(2) << summary.avgStyleScore << "\n";
        out << "      },\n";
        
        // Tweets
        out << "      \"tweets\": [\n";
        const auto& tweets = tweetsBySenator[summary.name];
        for (size_t j = 0; j < tweets.size(); j++) {
            const auto& t = tweets[j];
            out << "        {\n";
            out << "          \"tweetId\": \"" << escapeJsonString(t.tweetId) << "\",\n";
            out << "          \"datetime\": \"" << escapeJsonString(t.datetime) << "\",\n";
            out << "          \"text\": \"" << escapeJsonString(t.text) << "\",\n";
            out << "          \"totalWords\": " << t.totalWords << ",\n";
            out << "          \"posWordCount\": " << t.posWordCount << ",\n";
            out << "          \"negWordCount\": " << t.negWordCount << ",\n";
            out << "          \"baseSentimentScore\": " << fixed << setprecision(2) << t.baseSentimentScore << ",\n";
            out << "          \"adjustedSentimentScore\": " << fixed << setprecision(2) << t.adjustedSentimentScore << ",\n";
            out << "          \"negationHits\": " << t.negationHits << ",\n";
            out << "          \"intensifierHits\": " << t.intensifierHits << ",\n";
            out << "          \"downtonerHits\": " << t.downtonerHits << ",\n";
            out << "          \"exclamationCount\": " << t.exclamationCount << ",\n";
            out << "          \"questionCount\": " << t.questionCount << ",\n";
            out << "          \"allCapsWordCount\": " << t.allCapsWordCount << ",\n";
            out << "          \"emojiPositiveCount\": " << t.emojiPositiveCount << ",\n";
            out << "          \"emojiNegativeCount\": " << t.emojiNegativeCount << ",\n";
            out << "          \"slangPositiveCount\": " << t.slangPositiveCount << ",\n";
            out << "          \"slangNegativeCount\": " << t.slangNegativeCount << ",\n";
            out << "          \"ngramPositiveHits\": " << t.ngramPositiveHits << ",\n";
            out << "          \"ngramNegativeHits\": " << t.ngramNegativeHits << "\n";
            out << "        }";
            if (j < tweets.size() - 1) out << ",";
            out << "\n";
        }
        out << "      ]\n";
        out << "    }";
        if (i < advancedSummaries.size() - 1) out << ",";
        out << "\n";
    }
    
    out << "  ]\n";
    out << "}\n";
    
    out.close();
    cout << "Analysis exported to " << filename << endl;
}

// ============================================================================
// MAIN PROGRAM
// ============================================================================

int main()
{
    // Enable ANSI colors
    Color::enableColors();
    
    cout << "========================================" << endl;
    cout << "CS101 SENATOR TWEET SENTIMENT ANALYSIS" << endl;
    cout << "========================================\n" << endl;
    
    // Load data
    cout << "Loading data..." << endl;
    vector<vector<string>> rawTweets = read_tweets_csv_file();
    vector<Tweet> tweets = convertToTweets(rawTweets);
    cout << "Loaded " << tweets.size() << " tweets." << endl;
    
    vector<string> posWords = readEmotionFile("positive-words.txt");
    vector<string> negWords = readEmotionFile("negative-words.txt");
    cout << "Loaded " << posWords.size() << " positive words and " 
         << negWords.size() << " negative words." << endl;
    
    // Build lexicons
    cout << "Building lexicons..." << endl;
    unordered_set<string> positiveLexicon = buildLexiconSet(posWords);
    unordered_set<string> negativeLexicon = buildLexiconSet(negWords);
    
    // PART I: Compute base sentiment stats
    cout << "Computing base sentiment statistics..." << endl;
    vector<SenatorStats> baseStats = computeBaseSenatorStats(tweets, positiveLexicon, negativeLexicon);
    
    // Get unique senators for menu
    vector<string> senators = getUniqueSenators(tweets);
    sort(senators.begin(), senators.end());
    
    printBaseSentimentTable(baseStats);
    
    cout << "\nPress any key to continue to menu...";
    #ifdef _WIN32
    _getch();
    #else
    cin.get();
    #endif
    
    // Interactive menu
    while (true) {
        vector<string> mainMenuOptions = {
            "Show most positive/negative tweet for a senator",
            "Show most talkative senators",
            "Run advanced sentiment analysis + export JSON",
            "Exit"
        };
        
        int choice = ArrowMenu::showMenu("MAIN MENU", mainMenuOptions);
        
        if (choice == -1 || choice == 3) {
            cout << "\nGoodbye!" << endl;
            break;
        }
        else if (choice == 0) {
            // Show senator selection submenu
            vector<string> senatorMenuOptions = senators;
            senatorMenuOptions.push_back("Back to Main Menu");
            
            int senatorChoice = ArrowMenu::showMenu("SELECT A SENATOR", senatorMenuOptions);
            
            if (senatorChoice >= 0 && senatorChoice < senators.size()) {
                #ifdef _WIN32
                system("cls");
                #else
                system("clear");
                #endif
                showMostPositiveAndNegativeTweetForSenator(tweets, positiveLexicon, negativeLexicon, senators[senatorChoice]);
                cout << "\nPress any key to continue...";
                #ifdef _WIN32
                _getch();
                #else
                cin.get();
                #endif
            }
        }
        else if (choice == 1) {
            #ifdef _WIN32
            system("cls");
            #else
            system("clear");
            #endif
            vector<TalkStats> talkStats = computeTalkStats(tweets);
            printTalkStatsAndMostTalkative(talkStats);
            cout << "\nPress any key to continue...";
            #ifdef _WIN32
            _getch();
            #else
            cin.get();
            #endif
        }
        else if (choice == 2) {
            #ifdef _WIN32
            system("cls");
            #else
            system("clear");
            #endif
            
            cout << "\n========================================" << endl;
            cout << "ADVANCED SENTIMENT ANALYSIS" << endl;
            cout << "========================================\n" << endl;
            
            cout << "Building advanced sentiment lexicons..." << endl;
            
            unordered_map<string, double> wordPolarity = buildWordPolarityMap(posWords, negWords);
            unordered_set<string> negationWords = buildNegationWords();
            unordered_set<string> intensifiers = buildIntensifiers();
            unordered_set<string> downtoners = buildDowntoners();
            unordered_map<string, double> ngramPolarity = buildNgramPolarity();
            unordered_set<string> positiveEmojisSlang = buildPositiveEmojisSlang();
            unordered_set<string> negativeEmojisSlang = buildNegativeEmojisSlang();
            
            cout << "Analyzing all tweets with advanced sentiment engine..." << endl;
            vector<AdvancedTweetAnalysis> allAnalyses = analyzeAllTweetsAdvanced(
                tweets, wordPolarity, negationWords, intensifiers, downtoners,
                ngramPolarity, positiveEmojisSlang, negativeEmojisSlang
            );
            
            cout << "Summarizing by senator..." << endl;
            vector<AdvancedSenatorSummary> advancedSummaries = summarizeAdvancedBySenator(allAnalyses);
            
            cout << "Writing JSON export..." << endl;
            writeAnalysisJson(baseStats, advancedSummaries, allAnalyses, "analysis.json");
            
            cout << "\n========================================" << endl;
            cout << "ADVANCED SENTIMENT SUMMARY" << endl;
            cout << "========================================" << endl;
            cout << left << setw(25) << "Senator"
                 << right << setw(10) << "Tweets"
                 << setw(12) << "Avg Base"
                 << setw(12) << "Avg Adj"
                 << setw(10) << "Style" << endl;
            cout << string(69, '-') << endl;
            
            for (const auto& s : advancedSummaries) {
                cout << left << setw(25) << s.name
                     << right << setw(10) << s.tweetCount
                     << setw(12) << fixed << setprecision(2) << s.avgBaseSentiment
                     << setw(12) << fixed << setprecision(2) << s.avgAdjustedSentiment
                     << setw(10) << fixed << setprecision(1) << s.avgStyleScore << endl;
            }
            cout << endl;
            
            cout << "Analysis complete! JSON exported to analysis.json" << endl;
            cout << "\nPress any key to continue...";
            #ifdef _WIN32
            _getch();
            #else
            cin.get();
            #endif
        }
    }
    
    return 0;
}
