/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel Project                          *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) version 3.                                           *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "treemodel.h"

#include <QDebug>
#include <QCoreApplication>

/*****************************************
 *  Abstract Items of a TreeModel
 *****************************************/
AbstractTreeItem::AbstractTreeItem(AbstractTreeItem *parent)
  : QObject(parent),
    _flags(Qt::ItemIsSelectable | Qt::ItemIsEnabled)
{
}

AbstractTreeItem::~AbstractTreeItem() {
}

quint64 AbstractTreeItem::id() const {
  return qHash(this);
}

bool AbstractTreeItem::newChild(AbstractTreeItem *item) {
  // check if a child with that ID is already known
  Q_ASSERT(childById(item->id()) == 0);
    
  int newRow = childCount();
  emit beginAppendChilds(newRow, newRow);
  _childItems.append(item);
  emit endAppendChilds();
  
  return true;
}

bool AbstractTreeItem::newChilds(const QList<AbstractTreeItem *> &items) {
  if(items.isEmpty())
    return false;
  
  QList<AbstractTreeItem *>::const_iterator itemIter = items.constBegin();
  AbstractTreeItem *item;
  while(itemIter != items.constEnd()) {
    item = *itemIter;
    if(childById(item->id()) != 0) {
      qWarning() << "AbstractTreeItem::newChilds(): received child that is already attached" << item << item->id();
      return false;
    }
    itemIter++;
  }

  int nextRow = childCount();
  int lastRow = nextRow + items.count() - 1;

  emit beginAppendChilds(nextRow, lastRow);
  itemIter = items.constBegin();
  while(itemIter != items.constEnd()) {
    _childItems.append(*itemIter);
    itemIter++;
  }
  emit endAppendChilds();

  return true;
}

bool AbstractTreeItem::removeChild(int row) {
  if(childCount() <= row)
    return false;

  child(row)->removeAllChilds();
  emit beginRemoveChilds(row, row);
  AbstractTreeItem *treeitem = _childItems.takeAt(row);
  treeitem->deleteLater();
  emit endRemoveChilds();

  return true;
}

bool AbstractTreeItem::removeChildById(const quint64 &id) {
  const int numChilds = childCount();
  
  for(int i = 0; i < numChilds; i++) {
    if(_childItems[i]->id() == id)
      return removeChild(i);
  }
  
  return false;
}

void AbstractTreeItem::removeAllChilds() {
  const int numChilds = childCount();
  
  if(numChilds == 0)
    return;
  
  AbstractTreeItem *child;

  QList<AbstractTreeItem *>::iterator childIter;

  childIter = _childItems.begin();
  while(childIter != _childItems.end()) {
    child = *childIter;
    child->removeAllChilds();
    childIter++;
  }

  emit beginRemoveChilds(0, numChilds - 1);
  childIter = _childItems.begin();
  while(childIter != _childItems.end()) {
    child = *childIter;
    childIter = _childItems.erase(childIter);
    child->deleteLater();
  }
  emit endRemoveChilds();
}

AbstractTreeItem *AbstractTreeItem::child(int row) const {
  if(childCount() <= row)
    return 0;
  else
    return _childItems[row];
}

AbstractTreeItem *AbstractTreeItem::childById(const quint64 &id) const {
  const int numChilds = childCount();
  for(int i = 0; i < numChilds; i++) {
    if(_childItems[i]->id() == id)
      return _childItems[i];
  }
  return 0;
}

int AbstractTreeItem::childCount(int column) const {
  if(column > 0)
    return 0;
  else
    return _childItems.count();
}

int AbstractTreeItem::row() const {
  if(!parent())
    return -1;
  else
    return parent()->_childItems.indexOf(const_cast<AbstractTreeItem *>(this));
}

AbstractTreeItem *AbstractTreeItem::parent() const {
  return qobject_cast<AbstractTreeItem *>(QObject::parent());
}

Qt::ItemFlags AbstractTreeItem::flags() const {
  return _flags;
}

void AbstractTreeItem::setFlags(Qt::ItemFlags flags) {
  _flags = flags;
}

void AbstractTreeItem::dumpChildList() {
  qDebug() << "==== Childlist for Item:" << this << id() << "====";
  if(childCount() > 0) {
    AbstractTreeItem *child;
    QList<AbstractTreeItem *>::const_iterator childIter = _childItems.constBegin();
    while(childIter != _childItems.constEnd()) {
      child = *childIter;
      qDebug() << "Row:" << child->row() << child << child->id() << child->data(0, Qt::DisplayRole);
      childIter++;
    }
  }
  qDebug() << "==== End Of Childlist ====";  
}

/*****************************************
 * SimpleTreeItem
 *****************************************/
SimpleTreeItem::SimpleTreeItem(const QList<QVariant> &data, AbstractTreeItem *parent)
  : AbstractTreeItem(parent),
    _itemData(data)
{
}

SimpleTreeItem::~SimpleTreeItem() {
}

QVariant SimpleTreeItem::data(int column, int role) const {
  if(column >= columnCount() || role != Qt::DisplayRole)
    return QVariant();
  else
    return _itemData[column];
}

bool SimpleTreeItem::setData(int column, const QVariant &value, int role) {
  if(column > columnCount() || role != Qt::DisplayRole)
    return false;

  if(column == columnCount())
    _itemData.append(value);
  else
    _itemData[column] = value;

  emit dataChanged(column);
  return true;
}

int SimpleTreeItem::columnCount() const {
  return _itemData.count();
}

/*****************************************
 * PropertyMapItem
 *****************************************/
PropertyMapItem::PropertyMapItem(const QStringList &propertyOrder, AbstractTreeItem *parent)
  : AbstractTreeItem(parent),
    _propertyOrder(propertyOrder)
{
}

PropertyMapItem::PropertyMapItem(AbstractTreeItem *parent)
  : AbstractTreeItem(parent),
    _propertyOrder(QStringList())
{
}


PropertyMapItem::~PropertyMapItem() {
}
  
QVariant PropertyMapItem::data(int column, int role) const {
  if(column >= columnCount())
    return QVariant();

  switch(role) {
  case Qt::ToolTipRole:
    return toolTip(column);
  case Qt::DisplayRole:
  case TreeModel::SortRole:  // fallthrough, since SortRole should default to DisplayRole
    return property(_propertyOrder[column].toAscii());
  default:
    return QVariant();
  }
  
}

bool PropertyMapItem::setData(int column, const QVariant &value, int role) {
  if(column >= columnCount() || role != Qt::DisplayRole)
    return false;

  emit dataChanged(column);
  return setProperty(_propertyOrder[column].toAscii(), value);
}

int PropertyMapItem::columnCount() const {
  return _propertyOrder.count();
}
  
void PropertyMapItem::appendProperty(const QString &property) {
  _propertyOrder << property;
}



/*****************************************
 * TreeModel
 *****************************************/
TreeModel::TreeModel(const QList<QVariant> &data, QObject *parent)
  : QAbstractItemModel(parent),
    _childStatus(QModelIndex(), 0, 0, 0),
    _aboutToRemoveOrInsert(false)
{
  rootItem = new SimpleTreeItem(data, 0);
  connectItem(rootItem);

  if(QCoreApplication::instance()->arguments().contains("--debugmodel")) {
    connect(this, SIGNAL(rowsAboutToBeInserted(const QModelIndex &, int, int)),
	    this, SLOT(debug_rowsAboutToBeInserted(const QModelIndex &, int, int)));
    connect(this, SIGNAL(rowsAboutToBeRemoved(const QModelIndex &, int, int)),
	    this, SLOT(debug_rowsAboutToBeRemoved(const QModelIndex &, int, int)));
    connect(this, SIGNAL(rowsInserted(const QModelIndex &, int, int)),
	    this, SLOT(debug_rowsInserted(const QModelIndex &, int, int)));
    connect(this, SIGNAL(rowsRemoved(const QModelIndex &, int, int)),
	    this, SLOT(debug_rowsRemoved(const QModelIndex &, int, int)));
    connect(this, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
	    this, SLOT(debug_dataChanged(const QModelIndex &, const QModelIndex &)));
  }
}

TreeModel::~TreeModel() {
  delete rootItem;
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex &parent) const {
  if(!hasIndex(row, column, parent))
    return QModelIndex();
  
  AbstractTreeItem *parentItem;
  
  if(!parent.isValid())
    parentItem = rootItem;
  else
    parentItem = static_cast<AbstractTreeItem *>(parent.internalPointer());
  
  AbstractTreeItem *childItem = parentItem->child(row);

  if(childItem)
    return createIndex(row, column, childItem);
  else
    return QModelIndex();
}

QModelIndex TreeModel::indexById(quint64 id, const QModelIndex &parent) const {
  AbstractTreeItem *parentItem; 
  
  if(!parent.isValid())
    parentItem = rootItem;
  else
    parentItem = static_cast<AbstractTreeItem *>(parent.internalPointer());
  
  AbstractTreeItem *childItem = parentItem->childById(id);
  
  if(childItem)
    return createIndex(childItem->row(), 0, childItem);
  else
    return QModelIndex();
}

QModelIndex TreeModel::indexByItem(AbstractTreeItem *item) const {
  if(item == 0) {
    qWarning() << "TreeModel::indexByItem(AbstractTreeItem *item) received NULL-Pointer";
    return QModelIndex();
  }
  
  if(item == rootItem)
    return QModelIndex();
  else
    return createIndex(item->row(), 0, item);
}

QModelIndex TreeModel::parent(const QModelIndex &index) const {
  if(!index.isValid()) {
    qWarning() << "TreeModel::parent(): has been asked for the rootItems Parent!";
    return QModelIndex();
  }
  
  AbstractTreeItem *childItem = static_cast<AbstractTreeItem *>(index.internalPointer());
  AbstractTreeItem *parentItem = childItem->parent();

  Q_ASSERT(parentItem);
  if(parentItem == rootItem)
    return QModelIndex();
  
  return createIndex(parentItem->row(), 0, parentItem);
}

int TreeModel::rowCount(const QModelIndex &parent) const {
  AbstractTreeItem *parentItem;
  if(!parent.isValid())
    parentItem = rootItem;
  else
    parentItem = static_cast<AbstractTreeItem*>(parent.internalPointer());

  return parentItem->childCount(parent.column());
}

int TreeModel::columnCount(const QModelIndex &parent) const {
  Q_UNUSED(parent)
  return rootItem->columnCount();
  // since there the Qt Views don't draw more columns than the header has columns
  // we can be lazy and simply return the count of header columns
  // actually this gives us more freedom cause we don't have to ensure that a rows parent
  // has equal or more columns than that row

//   AbstractTreeItem *parentItem;
//   if(!parent.isValid())
//     parentItem = rootItem;
//   else
//     parentItem = static_cast<AbstractTreeItem*>(parent.internalPointer());
//   return parentItem->columnCount();
}

QVariant TreeModel::data(const QModelIndex &index, int role) const {
  if(!index.isValid())
    return QVariant();

  AbstractTreeItem *item = static_cast<AbstractTreeItem *>(index.internalPointer());
  if(role == Qt::DisplayRole && !item->data(index.column(), role).isValid()) {
    qDebug() << item->data(0, role) << item->columnCount();
  }
  return item->data(index.column(), role);
}

bool TreeModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  if(!index.isValid())
    return false;

  AbstractTreeItem *item = static_cast<AbstractTreeItem *>(index.internalPointer());
  return item->setData(index.column(), value, role);
}

Qt::ItemFlags TreeModel::flags(const QModelIndex &index) const {
  if(!index.isValid()) {
    return rootItem->flags() & Qt::ItemIsDropEnabled;
  } else {
    AbstractTreeItem *item = static_cast<AbstractTreeItem *>(index.internalPointer());
    return item->flags();
  }
}

QVariant TreeModel::headerData(int section, Qt::Orientation orientation, int role) const {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    return rootItem->data(section, role);
  else
    return QVariant();
}

void TreeModel::itemDataChanged(int column) {
  AbstractTreeItem *item = qobject_cast<AbstractTreeItem *>(sender());
  QModelIndex leftIndex, rightIndex;

  if(item == rootItem)
    return;

  if(column == -1) {
    leftIndex = createIndex(item->row(), 0, item);
    rightIndex = createIndex(item->row(), item->columnCount() - 1, item);
  } else {
    leftIndex = createIndex(item->row(), column, item);
    rightIndex = leftIndex;
  }

  emit dataChanged(leftIndex, rightIndex);
}

void TreeModel::connectItem(AbstractTreeItem *item) {
  connect(item, SIGNAL(dataChanged(int)),
	  this, SLOT(itemDataChanged(int)));
  
  connect(item, SIGNAL(beginAppendChilds(int, int)),
	  this, SLOT(beginAppendChilds(int, int)));
  connect(item, SIGNAL(endAppendChilds()),
	  this, SLOT(endAppendChilds()));
  
  connect(item, SIGNAL(beginRemoveChilds(int, int)),
	  this, SLOT(beginRemoveChilds(int, int)));
  connect(item, SIGNAL(endRemoveChilds()),
	  this, SLOT(endRemoveChilds()));
}

void TreeModel::beginAppendChilds(int firstRow, int lastRow) {
  AbstractTreeItem *parentItem = qobject_cast<AbstractTreeItem *>(sender());
  if(!parentItem) {
    qWarning() << "TreeModel::beginAppendChilds(): cannot append Childs to unknown parent";
    return;
  }

  QModelIndex parent = indexByItem(parentItem);
  Q_ASSERT(!_aboutToRemoveOrInsert);
  
  _aboutToRemoveOrInsert = true;
  _childStatus = ChildStatus(parent, rowCount(parent), firstRow, lastRow);
  beginInsertRows(parent, firstRow, lastRow);
}

void TreeModel::endAppendChilds() {
  AbstractTreeItem *parentItem = qobject_cast<AbstractTreeItem *>(sender());
  if(!parentItem) {
    qWarning() << "TreeModel::endAppendChilds(): cannot append Childs to unknown parent";
    return;
  }
  Q_ASSERT(_aboutToRemoveOrInsert);
  ChildStatus cs = _childStatus;
  QModelIndex parent = indexByItem(parentItem);
  Q_ASSERT(cs.parent == parent);
  Q_ASSERT(rowCount(parent) == cs.childCount + cs.end - cs.start + 1);

  _aboutToRemoveOrInsert = false;
  for(int i = cs.start; i <= cs.end; i++) {
    connectItem(parentItem->child(i));
  }
  endInsertRows();
}

void TreeModel::beginRemoveChilds(int firstRow, int lastRow) {
  AbstractTreeItem *parentItem = qobject_cast<AbstractTreeItem *>(sender());
  if(!parentItem) {
    qWarning() << "TreeModel::beginRemoveChilds(): cannot append Childs to unknown parent";
    return;
  }

  for(int i = firstRow; i <= lastRow; i++) {
    disconnect(parentItem->child(i), 0, this, 0);
  }
  
  // consitency checks
  QModelIndex parent = indexByItem(parentItem);
  Q_ASSERT(firstRow <= lastRow);
  Q_ASSERT(parentItem->childCount() > lastRow);
  Q_ASSERT(!_aboutToRemoveOrInsert);
  _aboutToRemoveOrInsert = true;
  _childStatus = ChildStatus(parent, rowCount(parent), firstRow, lastRow);

  beginRemoveRows(parent, firstRow, lastRow);
}

void TreeModel::endRemoveChilds() {
  AbstractTreeItem *parentItem = qobject_cast<AbstractTreeItem *>(sender());
  if(!parentItem) {
    qWarning() << "TreeModel::endRemoveChilds(): cannot remove Childs from unknown parent";
    return;
  }

  // concistency checks
  Q_ASSERT(_aboutToRemoveOrInsert);
  ChildStatus cs = _childStatus;
  QModelIndex parent = indexByItem(parentItem);
  Q_ASSERT(cs.parent == parent);
  Q_ASSERT(rowCount(parent) == cs.childCount - cs.end + cs.start - 1);
  _aboutToRemoveOrInsert = false;

  endRemoveRows();
}

void TreeModel::clear() {
  rootItem->removeAllChilds();
}

void TreeModel::debug_rowsAboutToBeInserted(const QModelIndex &parent, int start, int end) {
  qDebug() << "debug_rowsAboutToBeInserted" << parent << parent.internalPointer() << parent.data().toString() << rowCount(parent) << start << end;
}

void TreeModel::debug_rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end) {
  AbstractTreeItem *parentItem;
  parentItem = static_cast<AbstractTreeItem *>(parent.internalPointer());
  if(!parentItem)
    parentItem = rootItem;
  qDebug() << "debug_rowsAboutToBeRemoved" << parent << parentItem << parent.data().toString() << rowCount(parent) << start << end;

  QModelIndex child;
  AbstractTreeItem *childItem;
  for(int i = end; i >= start; i--) {
    child = parent.child(i, 0);
    childItem = parentItem->child(i);
    Q_ASSERT(childItem);
    qDebug() << ">>>" << i << child << childItem->id() << child.data().toString();
  }
}

void TreeModel::debug_rowsInserted(const QModelIndex &parent, int start, int end) {
  AbstractTreeItem *parentItem;
  parentItem = static_cast<AbstractTreeItem *>(parent.internalPointer());
  if(!parentItem)
    parentItem = rootItem;
  qDebug() << "debug_rowsInserted:" << parent << parentItem << parent.data().toString() << rowCount(parent) << start << end;

  QModelIndex child;
  AbstractTreeItem *childItem;
  for(int i = start; i <= end; i++) {
    child = parent.child(i, 0);
    childItem = parentItem->child(i);
    Q_ASSERT(childItem);
    qDebug() << "<<<" << i << child << childItem->id() << child.data().toString();
  }
}

void TreeModel::debug_rowsRemoved(const QModelIndex &parent, int start, int end) {
  qDebug() << "debug_rowsRemoved" << parent << parent.internalPointer() << parent.data().toString() << rowCount(parent) << start << end;
}

void TreeModel::debug_dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight) {
  qDebug() << "debug_dataChanged" << topLeft << bottomRight;
  QStringList displayData;
  for(int row = topLeft.row(); row <= bottomRight.row(); row++) {
    displayData = QStringList();
    for(int column = topLeft.column(); column <= bottomRight.column(); column++) {
      displayData << data(topLeft.sibling(row, column), Qt::DisplayRole).toString();
    }
    qDebug() << "  row:" << row << displayData;
  }
}
