/*
 * This file is part of Usbpp, a C++ wrapper around libusb
 * Copyright (C) 2015  Lukas Jirkovsky <l.jirkovsky @at@ gmail.com>
 *
 * Usbpp is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, version 3 of the License
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "hidreport.h"

#include <stack>
#include <utility>

namespace Usbpp {
namespace HID {

class ReportItem::Impl {
public:
	Format format; // short/long
	std::uint8_t dataSize; // bSize/bDataSize
	Type type; // bType
	std::uint8_t tag; // bTag/bLongItemTag
	std::size_t bytelen; // length of the binary representation in bytes
	ByteBuffer data;

	Impl();
	explicit Impl(const std::uint8_t* data_);
	Impl(const Impl &other);
	~Impl();
};

ReportItem::Impl::Impl() :
	format(Format::SHORT),
	dataSize(0),
	type(Type::MAIN),
	tag(0),
	bytelen(0) {

};

ReportItem::Impl::Impl(const Impl &other) :
	format(other.format),
	dataSize(other.dataSize),
	type(other.type),
	tag(other.tag),
	bytelen(other.bytelen),
	data(other.data) {

}

ReportItem::Impl::Impl(const std::uint8_t* data_) {
	std::uint8_t bSize = data_[0] & 0x3;
	std::uint8_t bType = (data_[0] >> 2) & 0x3;
	std::uint8_t bTag = (data_[0] >> 4) & 0xf;

	// long item
	if (bTag == 0xf) {
		if (bSize != 2) {
			/* error - bSize is 2 for long items */
		}
		format = Format::LONG;
		dataSize = data_[1];
		type = static_cast<Type>(bType);
		tag = data_[2];
		if (dataSize > 0) {
			data = ByteBuffer(&data_[3], dataSize);
		}
		/* move the iterator to the next item */
		bytelen = dataSize + 3;
	}
	// short item
	else {
		format = Format::SHORT;
		dataSize = bSize != 3 ? bSize : 4;
		type = static_cast<Type>(bType);
		tag = bTag;
		if (dataSize > 0) {
			data = ByteBuffer(&data_[1], dataSize);
		}
		/* move the iterator to the next item */
		bytelen = dataSize + 1;
	}
}

ReportItem::Impl::~Impl() {

}

class ReportNode::Impl {
public:
	// state
	GlobalItemMap globalState;
	LocalItemMap localState;
	// the actual item with data
	const ReportItem item;
	// structural members
	const Ptr parent;
	List children;

	Impl();
	Impl(const Impl &other);
	Impl(const Ptr &parent_,
	     const ReportItem &item_,
	     const GlobalItemMap &globalState_,
	     const LocalItemMap &localState_);
};

ReportNode::Impl::Impl() {

}

ReportNode::Impl::Impl(const Impl &other) :
	globalState(other.globalState),
	localState(other.localState),
	item(other.item),
	parent(other.parent),
	children(other.children) {

}

ReportNode::Impl::Impl(const Ptr &parent_,
                       const ReportItem &item_,
                       const GlobalItemMap &globalState_,
                       const LocalItemMap &localState_) :
	globalState(globalState_),
	localState(localState_),
	item(item_),
	parent(parent_) {

}

ReportItem::ReportItem() : pimpl(new Impl) {

}

ReportItem::ReportItem(const ReportItem &other) : pimpl(new Impl(*other.pimpl)) {

}

ReportItem::ReportItem(ReportItem &&other) : pimpl(std::move(other.pimpl)) {

}

ReportItem::~ReportItem() {

}

ReportItem::ReportItem(const std::uint8_t* data) : pimpl(new Impl(data)) {

}

ReportItem &ReportItem::operator=(const ReportItem &other) {
	if (this != &other) {
		ReportItem tmp(other);
		std::swap(pimpl, tmp.pimpl);
	}

	return *this;
}

ReportItem &ReportItem::operator=(ReportItem &&other) noexcept {
	if (this != &other) {
		pimpl = std::move(other.pimpl);
	}

	return *this;
}

ReportItem::Format ReportItem::getFormat() const {
	return pimpl->format;
}

std::uint8_t ReportItem::getDataSize() const {
	return pimpl->dataSize;
}

ReportItem::Type ReportItem::getType() const {
	return pimpl->type;
}

std::uint8_t ReportItem::getTag() const {
	return pimpl->tag;
}

const ByteBuffer &ReportItem::getData() const {
	return pimpl->data;
}

ReportNode::ReportNode() : pimpl(new Impl) {

}

ReportNode::ReportNode(const Ptr &parent_,
                       const ReportItem &item_,
                       const GlobalItemMap &globalState_,
                       const LocalItemMap &localState_) :
	pimpl(new Impl(parent_, item_, globalState_, localState_)) {

}

ReportNode::ReportNode(const ReportNode &other) : pimpl(new Impl(*other.pimpl)) {

}

ReportNode::ReportNode(ReportNode &&other) noexcept : pimpl(std::move(other.pimpl)) {

}

ReportNode::~ReportNode() {

}

ReportNode &ReportNode::operator=(const ReportNode &other) {
	if (this != &other) {
		ReportNode tmp(other);
		std::swap(pimpl, tmp.pimpl);
	}

	return *this;
}

ReportNode &ReportNode::operator=(ReportNode &&other) noexcept {
	if (this != &other) {
		pimpl = std::move(other.pimpl);
	}

	return *this;
}

const ReportNode::GlobalItemMap &ReportNode::getGlobalState() const {
	return pimpl->globalState;
}

const ReportNode::LocalItemMap &ReportNode::getLocalState() const {
	return pimpl->localState;
}
const ReportItem &ReportNode::getItem() const {
	return pimpl->item;
}

const ReportNode::Ptr &ReportNode::getParent() const {
	return pimpl->parent;
}

const ReportNode::List &ReportNode::getChildren() const {
	return pimpl->children;
}

ReportTree::ReportTree(const ByteBuffer& buffer) :
	root(std::make_shared<ReportNode>()) {

	typedef std::stack<ReportNode::GlobalItemMap> GlobalItemTableStack;
	GlobalItemTableStack globalItemTableStack;
	ReportNode::GlobalItemMap globalState;
	ReportNode::LocalItemMap localState;

	ReportNode::Ptr lastroot = root;

	for (std::size_t i = 0; i < buffer.size();) {
		// load the item
		ReportItem item(&(buffer.data()[i]));
		i += item.pimpl->bytelen;

		switch (item.getType()) {
			case ReportItem::Type::MAIN: {
				switch (static_cast<ReportItem::TagsMain>(item.getTag())) {
					case ReportItem::TagsMain::COLLECTION: {
						ReportNode::Ptr newnode(new ReportNode(lastroot, item, globalState, localState));
						lastroot->pimpl->children.push_back(newnode);
						lastroot = newnode;
						break;
					}
					case ReportItem::TagsMain::END_COLLECTION: {
						ReportNode::Ptr newnode(new ReportNode(lastroot, item, globalState, localState));
						lastroot->pimpl->children.push_back(newnode);
						lastroot = lastroot->pimpl->parent;
						break;
					}
					default: {
						ReportNode::Ptr newnode(new ReportNode(lastroot, item, globalState, localState));
						lastroot->pimpl->children.push_back(newnode);
						break;
					}
				}
				// reset the local state
				localState.clear();
				break;
			}
			case ReportItem::Type::GLOBAL: {
				switch (static_cast<ReportItem::TagsGlobal>(item.getTag())) {
					case ReportItem::TagsGlobal::PUSH:
						globalItemTableStack.push(globalState);
						break;
					case ReportItem::TagsGlobal::POP:
						if (globalItemTableStack.empty()) {
							globalState.clear();
						}
						else {
							globalState = globalItemTableStack.top();
							globalItemTableStack.pop();
						}
						break;
					default:
						globalState[static_cast<ReportItem::TagsGlobal>(item.getTag())] = item;
						break;
				}
				break;
			}
			case ReportItem::Type::LOCAL: {
				localState[static_cast<ReportItem::TagsLocal>(item.getTag())] = item;
				break;
			}
			default:
				/* reserved */
				;
		}
	}
}

ReportTree::ReportTree(ReportTree &&other) : root(std::move(other.root)) {

}

ReportTree &ReportTree::operator=(ReportTree &&other) {
	if (this != &other) {
		root = std::move(other.root);
	}

	return *this;
}

ReportTree::~ReportTree() {

}

ReportNode::Ptr ReportTree::getRoot() const {
	return root;
}

}
}
