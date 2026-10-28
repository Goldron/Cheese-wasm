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

#include "pgn.h"

#include "util/logfile.h"

#include <iostream>

namespace Cheese::ChessPGN {

///////////////////////////////////////////////////////////////////////////////
const char char_cr      			= static_cast<char>(13);
const char char_lf      			= static_cast<char>(10);
const char char_tab     			= static_cast<char>(9);
const char char_space				= ' ';
const char char_bracket_open		= '[';
const char char_bracket_close		= ']';
const char char_parentheses_open	= '(';
const char char_parentheses_close	= ')';
const char char_brace_open			= '{';
const char char_brace_close			= '}';
const char char_double_quote		= '"';
const char char_dot					= '.';
const char char_dollar				= '$';
const char char_hyphen				= '-';
const char char_slash				= '/';
const char char_star				= '*';

///////////////////////////////////////////////////////////////////////////////
constexpr bool is_digit(char c)
{
	return ((c >= '0') && (c <= '9'));
}

///////////////////////////////////////////////////////////////////////////////
constexpr bool is_letter(char c)
{
	return (((c >= 'a') && (c <= 'z')) ||
			((c >= 'A') && (c <= 'Z')));
}

///////////////////////////////////////////////////////////////////////////////
constexpr bool is_special(char c)
{
	return (static_cast<unsigned char>(c) <
			static_cast<unsigned char>(char_space));
}

////////////////////////////////////////////////////////////////////////////////
bool PGNLoader::loadIndex(const std::string &filename, 
	std::vector<PGNIndex> &index)
{
	std::ifstream ifs(filename, std::ifstream::in | std::ifstream::binary);
	if (!ifs.is_open()) {
		return false;
	}
	
	PGNParser parser(*this);
	
	return parser.parseIndex(ifs, index);
}
	
////////////////////////////////////////////////////////////////////////////////	
bool PGNParser::parseIndex(std::ifstream &ifs, std::vector<PGNIndex> &index)
{	
	buffer.reserve(size_buffer_read);
	ifs.read(buffer.data(), size_buffer_read);
	size_buffer = ifs.gcount();

	std::streamsize ipos = 0;
	std::streamsize isize = 0;

	std::size_t nbparam = 0;	
	std::size_t file_pos = 0;
	bool bracket = false;
	bool content = false;
	bool brace = false;
	bool start = false;
	
    while (size_buffer != 0U) {
		
		std::size_t fp = 0;
		while (parse_pos < size_buffer) {
	
			fp = file_pos + parse_pos;
			
			char c = getNextChar();
			if (c == 0) {
				break;
			}
		
			switch (c) {
				
				case char_bracket_open:
					if ((!brace) && (!bracket)) {
						bracket = true;
						start = true;
								
						if (content) {							
							isize = static_cast<std::streamsize>(fp) - ipos;
							index.emplace_back(ipos, isize);
							nbparam = 0;
							content = false;
						}

						if (nbparam == 0) {
							ipos = static_cast<std::streamsize>(fp);						
						}						
					}
					break;
					
				case char_bracket_close:
					if ((!brace) && (bracket)) {
						bracket = false;
						++nbparam;
					}
					break;

				case char_brace_open:
					brace = true;
					break;

				case char_brace_close:
					brace = false;
					break;
					
				case char_space:
					break;
				
				default:
					if ((start) && (!brace) && (!bracket)) {
						if (!content) {
							content = true;
						}
					} 
					break;
			}
		}
		
		file_pos += size_buffer;
		
		if (size_buffer == size_buffer_read) {
            ifs.read(buffer.data(), size_buffer_read);
			size_buffer = ifs.gcount();
            parse_pos = 0;
        } else {
			if (content) {							
				isize = static_cast<std::streamsize>(fp) - ipos;
				index.emplace_back(ipos, isize);
			}
            break;
        }
	}
	
	return true;
}

///////////////////////////////////////////////////////////////////////////////
bool PGNLoader::loadFromString(const std::string &str)
{
	PGNParser parser(*this);

	return parser.parseFromString(str);
}

///////////////////////////////////////////////////////////////////////////////
bool PGNLoader::load(const std::string &filename, std::streamsize pos, 
	std::streamsize size)
{
	std::ifstream ifs(filename, std::ios::binary | std::ios::in);
	if (!ifs.is_open()) {
		return false;
	}
		
	PGNParser parser(*this);
	
    return parser.parse(ifs, pos, size);
}

///////////////////////////////////////////////////////////////////////////////
bool PGNLoader::loadFromIndex(const std::string &filename, 
	const std::vector<PGNIndex> &index, std::size_t pos, std::size_t count)
{
	std::ifstream ifs(filename, std::ios::binary | std::ios::in);
	if (!ifs.is_open()) {
		return false;
	}

	PGNParser parser(*this);
	
    return parser.parse(ifs, index, pos, count);
}

///////////////////////////////////////////////////////////////////////////////
bool PGNParser::parse(std::ifstream &ifs, const std::vector<PGNIndex> &index, 
	std::size_t pos, std::size_t count)
{
	for (auto i=pos; i<pos+count; ++i) {

		const auto position = index[i].position;
		const auto size = index[i].size;

		bool r = parse(ifs, position, size);
		if (!r) {
			return false;
		}
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////
bool PGNParser::parse(std::ifstream &ifs, std::streamsize pos, 
	std::streamsize size)
{
	if (pos != 0) {
		ifs.seekg(pos, std::ios::beg);
		if (ifs.eof()) {
			return false;
		}
	}

	buffer.reserve(size);
   	ifs.read(buffer.data(), size);
	size_buffer = static_cast<std::size_t>(ifs.gcount());

	if (size_buffer != static_cast<std::size_t>(size)) {
		return false;
	}
	
	parse_pos = 0;
	count_moves = 0;

	return parseBuffer();
}

///////////////////////////////////////////////////////////////////////////////
bool PGNParser::parseFromString(const std::string &str)
{
	buffer.assign(str.begin(), str.end());
	size_buffer = buffer.size();

	cancel = false;
	parse_pos = 0;
	count_moves = 0;

	return parseBuffer();
}

///////////////////////////////////////////////////////////////////////////////
char PGNParser::getNextChar()
{
	char c = buffer[parse_pos++];
							
	switch (c) {
		
		// replace CR by space
		case char_cr:
			c = char_space;
			return c;
			
		// next line
		case char_lf:
			++line;
			column = 1;
			c = char_space;
			return c;
			
		// replace tab by spaces
		case char_tab:
			c = char_space;
			column += tab_size;
			return c;
			
		default:
			// replace other special characters by space
			if (!is_special(char_space)) {
				++column;
			} else {
				c = char_space;
			}
			return c;
	}
}

///////////////////////////////////////////////////////////////////////////////
void PGNParser::startParsingString(bool skip)
{
	// remember token position
	line_str_start = line;
	column_str_start = column - 1;	
	auto p = (skip) ? parse_pos : (parse_pos - 1);
	pos_str_start = p;
	pos_str_end = p;	
}

///////////////////////////////////////////////////////////////////////////////
std::string_view PGNParser::endParsingString(bool skip)
{
	auto pos = pos_str_start;
	auto count = (skip) ? (parse_pos - pos_str_start - 1) :
		(parse_pos - pos_str_start);	
	
	pos_str_end = parse_pos;
	pos_str_start = parse_pos;
	return { buffer.data() + pos, count };
}

///////////////////////////////////////////////////////////////////////////////
void PGNParser::cancelParsingString()
{
	pos_str_start = parse_pos;
	pos_str_end = parse_pos;
}

///////////////////////////////////////////////////////////////////////////////
bool PGNParser::parseBuffer()
{	
	parse_again = false;
	char c = 0;
    while ((!cancel) && (parse_pos < size_buffer)) {
		
		if (!parse_again) {
			c = getNextChar();
			if (c == 0) {
				break;
			}			
		}
		parse_again = false;
				
        switch (parse_mode) {

            case ParseMode::start:
                if (!parseModeStart(c)) {
					return false;
				}
                break;

            case ParseMode::declaration:
                if (!parseModeDeclaration(c)) {
					return false;
				}
                break;

            case ParseMode::comment:
                if (!parseModeComment(c)) {
					return false;
				}
                break;
					
            case ParseMode::variation:
                if (!parseModeVariation(c)) {
					return false;
				}
                break;					

            case ParseMode::token_num:
                if (!parseModeTokenNum(c)) {
					return false;
				}
                break;
					
			case ParseMode::token_move:
                if (!parseModeTokenMove(c)) {
					return false;
				}
                break;
					
			case ParseMode::token_result:
                if (!parseModeTokenResult(c)) {
					return false;
				}
                break;
					
            case ParseMode::token_dot:
                if (!parseModeTokenDot(c)) {
					return false;
				}
                break;					
					
			case ParseMode::token_nag:
                if (!parseModeTokenNag(c)) {
					return false;
				}
                break;										
					
            default:
				loader.onError(*this, "unknown parse mode", 
					line, column, c);
                return false;
        }
    }
	
    return true;
}

///////////////////////////////////////////////////////////////////////////////
bool PGNParser::parseModeStart(char c)
{
    switch (c) {

        // enter declaration
        case char_bracket_open:		
			parse_mode = ParseMode::declaration;
			startParsingString(true);
			level = 0;
            break;

        // enter comment
        case char_brace_open:
			parse_mode = ParseMode::comment;
			startParsingString(true);
            break;

        // enter variation
        case char_parentheses_open:
			parse_mode = ParseMode::variation;
			startParsingString(true);
			level = 0;
            break;
				
		// enter dots
		case char_dot:
			parse_mode = ParseMode::token_dot;
			startParsingString(false);
			break;
				
		// enter nag
		case char_dollar:
			parse_mode = ParseMode::token_nag;
			startParsingString(false);
			break;
				
		// enter result
		case char_star:
			parse_mode = ParseMode::token_result;
			startParsingString(false);
			break;
				
		// ignore spaces between elements
        case char_space:
            break;

        default:
			if (is_digit(c)) {
				parse_mode = ParseMode::token_num;
				startParsingString(false);
			} else {
				parse_mode = ParseMode::token_move;
				startParsingString(false);
			}
            break;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
bool PGNParser::parseModeDeclaration(char c)
{
    switch (c) {
		
		// test if we are inside double quotes
		// to allow brackets inside double quotes
		case char_double_quote:
			if (level == 0) {
				level = 1;
			} else {
				level = 0;
			}
			break;
		
		case char_bracket_open:
			if (level == 0) {
				return false;
			}
			break;
		
		case char_bracket_close:
			if (level == 0) {
				if (!skip_declarations) {
					if (!parseDeclaration(endParsingString(true))) {
						return false;
					}
				} else {
					cancelParsingString();
				}
				++count_declarations;
				parse_mode = ParseMode::start;
			}
            break;
			
        default:
                break;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
bool PGNParser::parseModeComment(char c)
{
    switch (c) {

        case char_brace_close:
			if (!skip_comments) {
				if (!parseComment(endParsingString(true))) {
					return false;
				}
			} else {
				cancelParsingString();
			}
			++count_comments;
			parse_mode = ParseMode::start;
            break;

        default:
            break;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
bool PGNParser::parseModeVariation(char c)
{
    switch (c) {

		case char_parentheses_open:
			++level;
			break;

        case char_parentheses_close:			
			if (level == 0) {					
				if (!skip_variations) {
					if (!parseVariation(endParsingString(true))) {
						return false;
					}					
				} else {
					cancelParsingString();
				}
				++count_variations;
				parse_mode = ParseMode::start;
			}
			--level;
            break;

        default:
            break;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
bool PGNParser::parseModeTokenNum(char c)
{
    switch (c) {

		case char_dot:
			if (!skip_num) {
				if (!parseTokenNum(endParsingString(true))) {
					return false;
				}				
			} else {
				cancelParsingString();
			}			
			parse_mode = ParseMode::start;
			parse_again = true;
            break;
	
        case char_space:	
			if (!skip_num) {
				if (!parseTokenNum(endParsingString(true))) {
					return false;
				}				
			} else {
				cancelParsingString();
			}
			parse_mode = ParseMode::start;
            break;
			
		case char_hyphen:
		case char_slash:
			parse_mode = ParseMode::token_result;
			break;

        default:	
            break;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
bool PGNParser::parseModeTokenMove(char c)
{
    switch (c) {

		case char_space:
			if (!skip_move) {
				if (!parseTokenMove(endParsingString(true))) {
					return false;
				}
				++count_moves;
			} else {
				cancelParsingString();
			}
			parse_mode = ParseMode::start;		
            break;
			
		default:
			break;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
bool PGNParser::parseModeTokenResult(char c)
{
    switch (c) {

		case char_space:
			if (!skip_result) {
				if (!parseTokenResult(endParsingString(true))) {
					return false;
				}				
			} else {
				cancelParsingString();
			}
			parse_mode = ParseMode::start;		
            break;
					
		default:
			break;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
bool PGNParser::parseModeTokenDot(char c)
{
    switch (c) {

		case char_dot:
			break;

        default:
			if (!skip_dot) {
				if (!parseTokenDot(endParsingString(true))) {
					return false;
				}				
			} else {
				cancelParsingString();
			}
			parse_mode = ParseMode::start;
			parse_again = true;
            break;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
bool PGNParser::parseModeTokenNag(char c)
{
    switch (c) {

		case char_space:
			if (!skip_nag) {
				if (!parseTokenNag(endParsingString(true))) {
					return false;
				}				
			} else {
				cancelParsingString();
			}
			parse_mode = ParseMode::start;		
            break;
				
        default:
            break;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
bool PGNParser::parseDeclaration(const std::string_view &str)
{
	if constexpr (use_parser_log) {
		loader.onLog(*this, std::string(str), line_str_start, column_str_start, 
			parse_mode);
	}
	
	auto p1 = str.find_first_of(char_space);
	if (p1 == std::string_view::npos) {
		loader.onError(*this, "bad declaration", line, column, str[0]);
		return false;
	}
	
	std::string_view name = str.substr(0, p1);
	
	auto p2 = str.find_first_not_of(char_space, p1);
	if (p2 == std::string_view::npos) {
		loader.onError(*this, "bad declaration", line, column, str[0]);
		return false;
	}
	
	if (str[p2] != char_double_quote) {
		loader.onError(*this, "bad declaration", line, column, str[0]);
		return false;
	}
	++p2;
	auto p3 = str.find_first_of(char_double_quote, p2);
	if (p3 == std::string_view::npos) {
		loader.onError(*this, "bad declaration", line, column, str[0]);
		return false;
	}
	
	std::string_view value = str.substr(p2, (p3 - p2));
	
	loader.onParseDeclaration(*this, name, value);

	if (parsing_game) {
		endGame();
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
bool PGNParser::parseTokenNum(const std::string_view &str)
{
	if (!parsing_game) {
		startGame();
	}
	
	if constexpr (use_parser_log) {
		loader.onLog(*this, std::string(str), line_str_start, column_str_start, 
			parse_mode);
	}
		
	return true;
}

///////////////////////////////////////////////////////////////////////////////
bool PGNParser::parseTokenMove(const std::string_view &str)
{
	if (!parsing_game) {
		startGame();
	}
	
	loader.onParseMove(*this, str);
	
	if constexpr (use_parser_log) {
		loader.onLog(*this, std::string(str), line_str_start, column_str_start, 
			parse_mode);
	}
		
	return true;
}

///////////////////////////////////////////////////////////////////////////////
bool PGNParser::parseTokenResult(const std::string_view &str)
{
	if constexpr (use_parser_log) {
		loader.onLog(*this, std::string(str), line_str_start, column_str_start, 
			parse_mode);
	}
	
	loader.onParseGameResult(*this, str);
		
	if (parsing_game) {
		endGame();
	}
			
	return true;
}

///////////////////////////////////////////////////////////////////////////////
bool PGNParser::parseTokenDot(const std::string_view &str)
{
	if constexpr (use_parser_log) {
		loader.onLog(*this, std::string(str), line_str_start, column_str_start, 
			parse_mode);
	}
	
	return true;
}

///////////////////////////////////////////////////////////////////////////////
bool PGNParser::parseTokenNag(const std::string_view &str)
{
	if constexpr (use_parser_log) {
		loader.onLog(*this, std::string(str), line_str_start, column_str_start, 
			parse_mode);
	}
	
	return true;
}

///////////////////////////////////////////////////////////////////////////////
bool PGNParser::parseComment(const std::string_view &str)
{
	if (!parsing_game) {
		startGame();
	}

	loader.onParseComment(*this, str);
	
	if constexpr (use_parser_log) {
		loader.onLog(*this, std::string(str), line_str_start, column_str_start, 
			parse_mode);
	}
		
	return true;
}

///////////////////////////////////////////////////////////////////////////////	
bool PGNParser::parseVariation(const std::string_view &str)
{
	if constexpr (use_parser_log) {
		loader.onLog(*this, std::string(str), line_str_start, column_str_start, 
			parse_mode);
	}	
	
	// TODO : parse variation content
	
	return true;
}

///////////////////////////////////////////////////////////////////////////////	
void PGNParser::startGame()
{
	parsing_game = true;
	loader.onStartGame(*this, count_games);
}

///////////////////////////////////////////////////////////////////////////////	
void PGNParser::endGame()
{
	parsing_game = false;
	loader.onEndGame(*this, count_games);
	++count_games;
}

///////////////////////////////////////////////////////////////////////////////	
std::string PGNLoader::create(const PGNFileData &game)
{
	std::string str;

	for (const auto & [name, value] : game.headers) {
		str += "[" + name + " \"" + value + "\"]\n";
	}

	str += '\n';

	std::string line, strmove;
		
	int ply = 1;
	for (const auto & [ move, comment ] : game.moves) {

		if (ply & 1) {
			strmove += std::to_string(1 + ply / 2) + ".";
		}
		strmove += move;
		if (!comment.empty()) {
			strmove += " {" + comment + "} ";
		} else {
			strmove += " ";
		}

		if ((line.size() + strmove.size()) < 80) {
			line += strmove;
		} else {
			str += line + '\n';
			line = strmove;
		}
		strmove = "";

		++ply;
	}

	if ((line.size() + strmove.size()) < 80) {
		line += strmove;
	} else {
		str += line + '\n';
		line = strmove;
	}

	str += line + '\n';
	str += game.result + '\n';

	return str;
}

} // namespace Cheese

