/*
 * Copyright © 2013 Jørgen Lind

 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.

 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
*/

#include "json_tree.h"

#include <stdio.h>
#include <assert.h>

namespace JT {

bool PrintBuffer::append(const char *data, size_t size)
{
    if (end + size > this->size)
        return false;

    memcpy(buffer + end, data, size);
    end += size;
    return true;
}

PrintHandler::PrintHandler(char *buffer, size_t size)
{
    m_buffers.push_back({buffer,size,0});
}

void PrintHandler::appendBuffer(char *buffer, size_t size)
{
    m_buffers.push_back({buffer,size,0});
}

void PrintHandler::markCurrentPrintBufferFull()
{
    m_finished_buffers.push_back(m_buffers.front());
    m_buffers.pop_front();
    if (m_buffers.size() == 0) {
        for (auto it = m_request_buffer_callbacks.begin(); it != m_request_buffer_callbacks.end(); ++it) {
            (*it)(this);
        }
    }
}

bool PrintHandler::canFit(size_t amount)
{
    while (m_buffers.size()) {
        if (currentPrintBuffer().canFit(amount))
            return true;
        markCurrentPrintBufferFull();
    }
    return false;
}

bool PrintHandler::write(const char *data, size_t size)
{
    if (!canFit(size))
        return false;
    currentPrintBuffer().append(data,size);
    return true;
}

const PrintBuffer &PrintHandler::firstFinishedBuffer() const
{
    if (m_finished_buffers.size())
        return m_finished_buffers.front();
    return m_buffers.front();
}

Node::Node(Node::Type type)
    : m_type(type)
{ }
Node::~Node()
{
}

StringNode *Node::stringNodeAt(const std::string &path) const
{
    Node *node = nodeAt(path);
    if (node)
        return node->asStringNode();
    return nullptr;
}

NumberNode *Node::numberNodeAt(const std::string &path) const
{
    Node *node = nodeAt(path);
    if (node)
        return node->asNumberNode();
    return nullptr;
}

BooleanNode *Node::booleanNodeAt(const std::string &path) const
{
    Node *node = nodeAt(path);
    if (node)
        return node->asBooleanNode();
    return nullptr;
}

NullNode *Node::nullNodeAt(const std::string &path) const
{
    Node *node = nodeAt(path);
    if (node)
        return node->asNullNode();
    return nullptr;
}

ArrayNode *Node::arrayNodeAt(const std::string &path) const
{
    Node *node = nodeAt(path);
    if (node)
        return node->asArrayNode();
    return nullptr;
}

ObjectNode *Node::objectNodeAt(const std::string &path) const
{
    Node *node = nodeAt(path);
    if (node)
        return node->asObjectNode();
    return nullptr;
}

Node *Node::nodeAt(const std::string &path) const
{
    return nullptr;
}

StringNode *Node::asStringNode()
{
    if (m_type == String)
        return static_cast<StringNode *>(this);
    return nullptr;
}

const StringNode *Node::asStringNode() const
{
    if (m_type == String)
        return static_cast<const StringNode *>(this);
    return nullptr;
}

NumberNode *Node::asNumberNode()
{
    if (m_type == Number)
        return static_cast<NumberNode *>(this);
    return nullptr;
}

const NumberNode *Node::asNumberNode() const
{
    if (m_type == Number)
        return static_cast<const NumberNode*>(this);
    return nullptr;
}

BooleanNode *Node::asBooleanNode()
{
    if (m_type == Bool)
        return static_cast<BooleanNode *>(this);
    return nullptr;
}

const BooleanNode *Node::asBooleanNode() const
{
    if (m_type == Bool)
        return static_cast<const BooleanNode *>(this);
    return nullptr;
}

NullNode *Node::asNullNode()
{
    if (m_type == Null)
        return static_cast<NullNode *>(this);
    return nullptr;
}

const NullNode *Node::asNullNode() const
{
    if (m_type == Null)
        return static_cast<const NullNode *>(this);
    return nullptr;
}

ArrayNode *Node::asArrayNode()
{
    if (m_type == Array)
        return static_cast<ArrayNode *>(this);
    return nullptr;
}

const ArrayNode *Node::asArrayNode() const
{
    if (m_type == Array)
        return static_cast<const ArrayNode *>(this);
    return nullptr;
}

ObjectNode *Node::asObjectNode()
{
    if (m_type == Object)
        return static_cast<ObjectNode *>(this);
    return nullptr;
}

const ObjectNode *Node::asObjectNode() const
{
    if (m_type == Object)
        return static_cast<const ObjectNode *>(this);
    return nullptr;
}

ObjectNode::ObjectNode()
    : Node(Node::Object)
{
}

ObjectNode::~ObjectNode()
{
    for (auto it = m_map.begin();
            it != m_map.end(); it++) {
       delete it->second;
    }
}

Node *ObjectNode::nodeAt(const std::string &path) const
{
    size_t first_dot = path.find('.');

    if (first_dot == 0)
        return nullptr;

    if (first_dot == std::string::npos) {
        auto it = m_map.find(path);
        if (it == m_map.end())
            return nullptr;
        else return it->second;
    }

    std::string first_node(path.substr(0,first_dot));
    auto it = m_map.find(first_node);
    if (it == m_map.end())
        return nullptr;
    Node *child_node = it->second;
    return child_node->nodeAt(path.substr(first_dot+1));
}

Node *ObjectNode::node(const std::string &child_node) const
{
    auto it = m_map.find(child_node);
    if (it == m_map.end())
        return nullptr;
    return it->second;
}

void ObjectNode::insertNode(const std::string &name, Node *node, bool replace)
{
    auto ret = m_map.insert(std::pair<std::string, Node *>(name, node));
    if (ret.second == false && replace) {
        delete ret.first->second;
        m_map.erase(ret.first);
        ret = m_map.insert(std::pair<std::string, Node *>(name, node));
        assert(ret.second == true);
    }
}

Node *ObjectNode::take(const std::string &name)
{
    auto it = m_map.find(name);
    if (it == m_map.end())
        return nullptr;
    Node *child_node = it->second;
    m_map.erase(it);
    return child_node;
}

Error ObjectNode::fill(Tokenizer *tokenizer)
{
    Token token;
    Error error;
    while ((error = tokenizer->nextToken(&token)) == Error::NoError) {
        if (token.data_type == Token::ObjectEnd) {
            return Error::NoError;
        }
        auto created = Node::create(&token, tokenizer);
        if (created.second != Error::NoError) {
            return created.second;
        }

        assert(token.name_length);
        insertNode(std::string(token.name, token.name_length), created.first, true);
    }
    return error;
}

size_t ObjectNode::printSize(const PrinterOption &option, int depth)
{
    depth++;
    size_t return_size = 0;
    bool first = true;
    int shift_width = depth * option.shiftSize();

    if (option.pretty()) {
        return_size += 2;
    } else {
        return_size++;
    }

    for (auto it = m_map.begin(); it != m_map.end(); ++it) {
        const std::string &property = (*it).first;
        if (first) {
            first = false;
        } else {
            if (option.pretty()) {
                return_size+=2;
            } else {
                return_size++;
            }
        }
        if (option.pretty()) {
            return_size += shift_width;
        }
        return_size += property.size() + 2;

        if (option.pretty()) {
            return_size += 3;
        } else {
            return_size += 1;
        }

        return_size += (*it).second->printSize(option,depth);
    }
    if (option.pretty()) {
        return_size += 1 + (shift_width - option.shiftSize());
    }
    return_size += 1;
    return return_size;
}

bool ObjectNode::print(PrintHandler &buffers, const PrinterOption &option , int depth)
{
    depth++;
    if (option.pretty()) {
        if (!buffers.write("{\n",2))
            return false;
    } else {
        if (!buffers.write("{", 1))
            return false;
    }

    int shift_width = option.shiftSize() * depth;

    for (auto it = m_map.begin(); it != m_map.end(); ++it) {
        const std::string &property = (*it).first;
        if (it != m_map.begin()) {
            if (option.pretty()) {
                if (!buffers.write(",\n",2))
                    return false;
           } else {
                if (!buffers.write(",",1))
                    return false;
            }
        }
        if (option.pretty()) {
            if (!buffers.write(
                        std::string(shift_width,' ').c_str(),shift_width))
                return false;
        }
        if (!buffers.write("\"",1))
            return false;
        if (!buffers.write(property.c_str(), property.size()))
            return false;
        if (!buffers.write("\"",1))
            return false;
        if (option.pretty()) {
            const char delimiter[] = " : ";
            if (!buffers.write(delimiter, 3))
                return false;
        } else {
            const char delimiter[] = ":";
            if (!buffers.write(delimiter, 1))
                return false;
        }
        if (!(*it).second->print(buffers,option,depth))
            return false;
    }

    if (option.pretty()) {
        std::string before_close_bracket("\n");
        before_close_bracket.append(std::string(shift_width - option.shiftSize(),' '));
        if (!buffers.write(before_close_bracket.c_str(), before_close_bracket.size()))
            return false;
    }
    if (!buffers.write("}",1))
        return false;
    return true;
}

StringNode::StringNode(Token *token)
    : Node(String)
    , m_string(token->data, token->data_length)
{
}

const std::string &StringNode::string() const
{
    return m_string;
}

void StringNode::setString(const std::string &string)
{
    m_string = string;
}

size_t StringNode::printSize(const PrinterOption &option, int depth)
{
    return m_string.size() + 2;
}

bool StringNode::print(PrintHandler &buffers, const PrinterOption &option , int depth)
{
    if (!buffers.write("\"",1))
        return false;
    if (!buffers.write(m_string.c_str(), m_string.size()))
        return false;
    if (!buffers.write("\"",1))
        return false;
    return true;
}

NumberNode::NumberNode(Token *token)
    : Node(Number)
{
    std::string null_terminated(token->data, token->data_length);
    char **success = 0;
    m_number = strtod(null_terminated.c_str(), success);
    if ((char *)success == null_terminated.c_str()) {
        fprintf(stderr, "numbernode failed to convert token to double\n");
    }
}

size_t NumberNode::printSize(const PrinterOption &option, int depth)
{
    char buff[20];
    size_t size  = snprintf(buff, sizeof(buff), "%f", m_number);
    assert(size > 0);
    return size;
}

bool NumberNode::print(PrintHandler &buffers, const PrinterOption &option , int depth)
{
    char buff[20];
    size_t size  = snprintf(buff, sizeof(buff), "%f", m_number);
    return buffers.write(buff,size);
}

BooleanNode::BooleanNode(Token *token)
    : Node(Bool)
{
    if (*token->data == 't' || *token->data == 'T')
        m_boolean = true;
    else
        m_boolean = false;
}

size_t BooleanNode::printSize(const PrinterOption &option, int depth)
{
    return m_boolean ? 4 : 5;
}

bool BooleanNode::print(PrintHandler &buffers, const PrinterOption &option , int depth)
{
    if (m_boolean)
        return buffers.write("true",4);
    else
        return buffers.write("false",5);
}

NullNode::NullNode(Token *token)
    : Node(Null)
{ }

size_t NullNode::printSize(const PrinterOption &option, int depth)
{
    return 4;
}

bool NullNode::print(PrintHandler &buffers, const PrinterOption &option , int depth)
{
    return buffers.write("null",4);
}

ArrayNode::ArrayNode()
    : Node(Array)
{
}

ArrayNode::~ArrayNode()
{
    for (auto it = m_vector.begin(); it != m_vector.end(); it++) {
        delete *it;
    }
}

void ArrayNode::insert(Node *node, size_t index)
{
    if (index >= m_vector.size()) {
        m_vector.push_back(node);
        return;
    }

    auto it = m_vector.begin();
    m_vector.insert(it + index, node);
}

void ArrayNode::append(Node *node)
{
    m_vector.push_back(node);
}

Node *ArrayNode::index(size_t index)
{
    if (index >= m_vector.size()) {
        return nullptr;
    }
    auto it = m_vector.begin();
    return *(it+index);
}

Node *ArrayNode::take(size_t index)
{
    if (index >= m_vector.size()) {
        return nullptr;
    }

    auto it = m_vector.begin();
    Node *return_node = *(it + index);
    m_vector.erase(it+index);
    return return_node;
}

size_t ArrayNode::size()
{
    return m_vector.size();
}

Error ArrayNode::fill(Tokenizer *tokenizer)
{
    Token token;
    Error error;
    while ((error = tokenizer->nextToken(&token)) == Error::NoError) {
        if (token.data_type == Token::ArrayEnd) {
            return Error::NoError;
        }
        auto created = Node::create(&token, tokenizer);

        if (created.second != Error::NoError)
            return created.second;

        m_vector.push_back(created.first);
    }

    return error;
}

size_t ArrayNode::printSize(const PrinterOption &option, int depth)
{
    depth++;
    int shift_width = option.shiftSize() * depth;
    size_t return_size = 0;

    if (option.pretty()) {
        return_size += 2;
    }else {
        return_size++;
    }

    bool first = true;
    for (auto it = m_vector.begin(); it != m_vector.end(); ++it) {
        if (first) {
            first = false;
        } else {
            if (option.pretty())
                return_size += 2;
            else
                return_size++;
        }
        if (option.pretty()) {
            return_size += shift_width;
        }

        return_size += (*it)->printSize(option, depth);
    }

    if (option.pretty()) {
        return_size += 1 + (shift_width - option.shiftSize());
    }

    return_size++;

    return return_size;
}

bool ArrayNode::print(PrintHandler &buffers, const PrinterOption &option , int depth)
{
    depth++;
    if (option.pretty()) {
        if (!buffers.write("[\n",2))
            return false;
    } else {
        if (!buffers.write("[",1))
            return false;
    }

    int shift_width = option.shiftSize() * depth;

    for (auto it = m_vector.begin(); it != m_vector.end(); ++it) {
        if (it != m_vector.begin()) {
            if (option.pretty()) {
                if (!buffers.write(",\n",2))
                    return false;
            } else {
                if (!buffers.write(",",1))
                    return false;
            }
        }
        if (option.pretty()) {
            if (!buffers.write(std::string(shift_width,' ').c_str(),shift_width))
                return false;
        }
        if (!(*it)->print(buffers,option,depth))
            return false;
    }

    if (option.pretty()) {
        if (!buffers.write("\n",1))
            return false;
        if (!buffers.write(std::string(shift_width - option.shiftSize(),' ').c_str(),shift_width - option.shiftSize()))
            return false;
    }
    if (!buffers.write("]",1))
        return false;
    return true;

}

std::pair<Node *, Error> Node::create(Token *token, Tokenizer *tokenizer)
{
    std::pair<Node *, Error> ret(nullptr, Error::NoError);
    switch (token->data_type) {
        case Token::ArrayStart:
            {
                ArrayNode *return_node = new ArrayNode();
                ret.first = return_node;
                ret.second = return_node->fill(tokenizer);
            }
            break;
        case Token::ObjectStart:
            {
                ObjectNode *return_node = new ObjectNode();
                ret.first = return_node;
                ret.second = return_node->fill(tokenizer);
            }
            break;
        case Token::String:
        case Token::Ascii:
            ret.first = new StringNode(token);
            break;
        case Token::Number:
            ret.first = new NumberNode(token);
            break;
        case Token::Bool:
            ret.first = new BooleanNode(token);
            break;
        case Token::Null:
            ret.first = new NullNode(token);
            break;
        default:
            break;
    }
    return ret;
}

std::pair<Node *, Error> Node::create(Tokenizer *tokenizer)
{
    Token token;
    auto token_error = tokenizer->nextToken(&token);
    if (token_error != Error::NoError) {
        return std::pair<Node *, Error>(nullptr, token_error);
    }
    return Node::create(&token, tokenizer);
}

} //namespace JT
