////////////////////////////////////////////////////////////////////////////////
//
// Cheese, Chess engine.
//
// Copyright (C) 2006-2025 Patrice Duhamel. All rights reserved.
//
// Cheese is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Cheese is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef CHEESE_CHESS_PGN_H_
#define CHEESE_CHESS_PGN_H_

#include <string>
#include <vector>
#include <array>
#include <fstream>
#include <functional>
#include <utility>
#include <cinttypes>

namespace Cheese::ChessPGN {

constexpr bool use_parser_log {false};

///////////////////////////////////////////////////////////////////////////////
// pgn read buffer size
constexpr std::size_t size_buffer_read = 8192U;

constexpr std::size_t default_tab_size = 1;

///////////////////////////////////////////////////////////////////////////////
enum class ParseMode {
	start,
	declaration,
	comment,
	variation,
	token_num,
	token_move,
	token_result,
	token_dot, 
	token_nag,
	max
};

constexpr std::size_t count_parse_mode = 
	static_cast<std::size_t>(ParseMode::max);

const std::array<std::string_view, count_parse_mode> str_parse_mode = {
	"start",
	"declaration",
	"comment",
	"variation",
	"number",
	"move",
	"result",
	"dot",
	"nag"
};

///////////////////////////////////////////////////////////////////////////////
// Position and size of a game in a file
struct PGNIndex {

	std::streamsize position;
	
	std::streamsize size;

	PGNIndex(std::streamsize p, std::streamsize s) : position(p), size(s)
	{ }
};

class PGNLoader;

///////////////////////////////////////////////////////////////////////////////
class PGNParser {

	PGNLoader &loader;
	
	// current read buffer
	std::vector<char> buffer;
		
	// current read buffer size
	std::size_t size_buffer {0};
	
	// parsing position in buffer
	std::size_t parse_pos {0};
	
	// current line in file
	std::size_t line {1};
	
	// current column in file
	std::size_t column {1};
	
	// current parsing mode
	ParseMode parse_mode {ParseMode::start};
	
	// current string parsed start position in buffer
	std::size_t pos_str_start {0};

	// current string parsed end position i buffer
	std::size_t pos_str_end {0};
	
	std::size_t line_str_start {0};
	
	std::size_t column_str_start {0};
	
	// level of parentheses
	int level {0};
	
	// count games parsed
	std::size_t count_games {0};
	
	// count moves parsed
	std::size_t count_moves {0};
		
	std::size_t count_comments {0};
	
	std::size_t count_variations {0};
	
	std::size_t count_declarations {0};
			
	std::size_t tab_size {default_tab_size};

	// we are parsing game content
	bool parsing_game {false};
	
	// to re-parse current char
	bool parse_again {false};

	bool skip_comments {true};
	
	bool skip_variations {true};
	
	bool skip_declarations {false};
	
	bool skip_num {true};
	
	bool skip_move {false};
	
	bool skip_dot {true};
	
	bool skip_nag {true};
	
	bool skip_result {false};	
		
	bool cancel {false};
		
public:	
	
	PGNParser(PGNLoader &l) : loader(l)
	{ }
	
	[[nodiscard]]
	bool parseIndex(std::ifstream &ifs, std::vector<PGNIndex> &index);
	
	[[nodiscard]]
	bool parse(std::ifstream &ifs, std::streamsize pos, std::streamsize size);

	[[nodiscard]]
	bool parse(std::ifstream &ifs, const std::vector<PGNIndex> &index, 
		std::size_t pos=0, std::size_t count=0);

	[[nodiscard]]
	bool parseFromString(const std::string &str);

	void setTabSize(std::size_t s);
	
	// stop parser as soon as possible
	void cancelParsing();
	
	[[nodiscard]]
	std::size_t getGameCount() const;

	[[nodiscard]]
	std::size_t getMoveCount() const;
	
	[[nodiscard]]
	std::size_t getCommentCount() const;
	
	[[nodiscard]]
	std::size_t getVariationCount() const;
	
	[[nodiscard]]
	std::size_t getDeclarationCount() const;
	
	// get current parser line
	[[nodiscard]]
	std::size_t getLine() const;
	
	// get current parser column
	[[nodiscard]]
	std::size_t getColumn() const;

	// get current token start line
	[[nodiscard]]
	std::size_t getTokenLine() const;
	
	// get current token start column
	[[nodiscard]]
	std::size_t getTokenColumn() const;

private:

	[[nodiscard]]
	bool parseBuffer();

	[[nodiscard]]
	char getNextChar();
	
	void startParsingString(bool skip);
	
	[[nodiscard]]	
	std::string_view endParsingString(bool skip);
	
	void cancelParsingString();
			
	[[nodiscard]]
	bool parseModeStart(char c);

	[[nodiscard]]
	bool parseModeDeclaration(char c);

	[[nodiscard]]
	bool parseModeComment(char c);
	
	[[nodiscard]]
	bool parseModeVariation(char c);

	[[nodiscard]]
	bool parseModeTokenNum(char c);
	
	[[nodiscard]]
	bool parseModeTokenMove(char c);
	
	[[nodiscard]]
	bool parseModeTokenResult(char c);
	
	[[nodiscard]]
	bool parseModeTokenDot(char c);
	
	[[nodiscard]]
	bool parseModeTokenNag(char c);

	[[nodiscard]]
	bool parseDeclaration(const std::string_view &str);
	
	[[nodiscard]]
	bool parseComment(const std::string_view &str);
	
	[[nodiscard]]
	bool parseVariation(const std::string_view &str);
	
	[[nodiscard]]
	bool parseTokenNum(const std::string_view &str);
	
	[[nodiscard]]
	bool parseTokenMove(const std::string_view &str);
	
	[[nodiscard]]
	bool parseTokenResult(const std::string_view &str);
	
	[[nodiscard]]
	bool parseTokenDot(const std::string_view &str);
	
	[[nodiscard]]
	bool parseTokenNag(const std::string_view &str);
	
	void startGame();
	
	void endGame();
};

///////////////////////////////////////////////////////////////////////////////
struct PGNFileHeader {

	std::string name;

	std::string value;

	PGNFileHeader(std::string n, std::string v) : name(n), value(v)
	{ }
};

struct PGNFileMove {

	std::string move;

	std::string comment;

	PGNFileMove(std::string m, std::string c) : move(m), comment(c)
	{ }
};

struct PGNFileData {

	std::vector<PGNFileHeader> headers;

	std::vector<PGNFileMove> moves;

	std::string result;
};

///////////////////////////////////////////////////////////////////////////////
// load PGN files
// - no move verifications
// - replace special characters by spaces
// - don't parse variations
class PGNLoader {

public:

    PGNLoader() = default;

	virtual ~PGNLoader() = default;

	// create an index with position and size of all games in a file
	[[nodiscard]]
	bool loadIndex(const std::string &filename, 
		std::vector<PGNIndex> &index);

	// load a PGN file from position and size in a file
	[[nodiscard]]
	bool load(const std::string &filename, std::streamsize pos, 
		std::streamsize size);

	// load a PGN file from string
	[[nodiscard]]
	bool loadFromString(const std::string &str);
					
	// load PGN files from an index
	[[nodiscard]]
	bool loadFromIndex(const std::string &filename, 
		const std::vector<PGNIndex> &index, 
		std::size_t pos=0, std::size_t count=0);

	std::string create(const PGNFileData &game);

    virtual void onStartGame(
		[[maybe_unused]] PGNParser &parser,
		[[maybe_unused]] std::size_t id)
    { }

    virtual void onEndGame(
		[[maybe_unused]] PGNParser &parser,
		[[maybe_unused]] std::size_t id)
    { }
	
	virtual void onParseDeclaration(
		[[maybe_unused]] PGNParser &parser,
		[[maybe_unused]] const std::string_view &name,
		[[maybe_unused]] const std::string_view &value)
    { }
	
	virtual void onParseMove(
		[[maybe_unused]] PGNParser &parser,
		[[maybe_unused]] const std::string_view &move)
    { }

	virtual void onParseComment(
		[[maybe_unused]] PGNParser &parser,
		[[maybe_unused]] const std::string_view &comment)
    { }
	
	virtual void onParseGameResult(
		[[maybe_unused]] PGNParser &parser,
		[[maybe_unused]] const std::string_view &result)
    { }
	
	virtual void onError(
		[[maybe_unused]] PGNParser &parser,
		[[maybe_unused]] const std::string &str, 
		[[maybe_unused]] std::size_t line, 
		[[maybe_unused]] std::size_t col,
		[[maybe_unused]] char c)
	{ }
	
	virtual void onLog(
		[[maybe_unused]] PGNParser &parser,
		[[maybe_unused]] const std::string &str, 
		[[maybe_unused]] std::size_t line, 
		[[maybe_unused]] std::size_t col,
		[[maybe_unused]] ParseMode mode)
	{ }
};

////////////////////////////////////////////////////////////////////////////////
inline void PGNParser::setTabSize(std::size_t s)
{
	tab_size = s;
}

////////////////////////////////////////////////////////////////////////////////
inline void PGNParser::cancelParsing()
{
	cancel = true;
}

////////////////////////////////////////////////////////////////////////////////
inline std::size_t PGNParser::getGameCount() const
{
	return count_games;
}

////////////////////////////////////////////////////////////////////////////////
inline std::size_t PGNParser::getMoveCount() const
{
	return count_moves;
}

////////////////////////////////////////////////////////////////////////////////
inline std::size_t PGNParser::getCommentCount() const
{
	return count_comments;
}

////////////////////////////////////////////////////////////////////////////////
inline std::size_t PGNParser::getVariationCount() const
{
	return count_variations;
}

////////////////////////////////////////////////////////////////////////////////
inline std::size_t PGNParser::getDeclarationCount() const
{
	return count_declarations;
}

////////////////////////////////////////////////////////////////////////////////
inline std::size_t PGNParser::getLine() const
{
	return line;
}

////////////////////////////////////////////////////////////////////////////////
inline std::size_t PGNParser::getColumn() const
{
	return column;
}

////////////////////////////////////////////////////////////////////////////////
inline std::size_t PGNParser::getTokenLine() const
{
	return line_str_start;
}

////////////////////////////////////////////////////////////////////////////////
inline std::size_t PGNParser::getTokenColumn() const
{
	return column_str_start;
}


} // namespace Cheese::ChessPGN

#endif // CHEESE_CHESS_PGN_H_

