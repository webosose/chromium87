/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// @ts-nocheck
// TODO(crbug.com/1011811): Enable TypeScript compiler checks

import * as Common from '../common/common.js';
import * as DataGrid from '../data_grid/data_grid.js';
import * as HeapSnapshotModel from '../heap_snapshot_model/heap_snapshot_model.js';
import * as Platform from '../platform/platform.js';
import * as SDK from '../sdk/sdk.js';
import * as UI from '../ui/ui.js';

import {ChildrenProvider} from './ChildrenProvider.js';  // eslint-disable-line no-unused-vars
import {AllocationDataGrid, HeapSnapshotConstructorsDataGrid, HeapSnapshotDiffDataGrid, HeapSnapshotRetainmentDataGrid, HeapSnapshotSortableDataGrid,} from './HeapSnapshotDataGrids.js';  // eslint-disable-line no-unused-vars
import {HeapSnapshotProviderProxy, HeapSnapshotProxy} from './HeapSnapshotProxy.js';  // eslint-disable-line no-unused-vars
import {DataDisplayDelegate} from './ProfileHeader.js';  // eslint-disable-line no-unused-vars

/**
 * @unrestricted
 */
export class HeapSnapshotGridNode extends DataGrid.DataGrid.DataGridNode {
  /**
   * @param {!HeapSnapshotSortableDataGrid} tree
   * @param {boolean} hasChildren
   */
  constructor(tree, hasChildren) {
    super(null, hasChildren);
    this._dataGrid = tree;
    this._instanceCount = 0;

    this._savedChildren = null;
    /**
     * List of position ranges for all visible nodes: [startPos1, endPos1),...,[startPosN, endPosN)
     * Position is an item position in the provider.
     */
    this._retrievedChildrenRanges = [];

    /**
     * @type {?ChildrenProvider}
     */
    this._providerObject = null;
    this._reachableFromWindow = false;
  }

  /**
   * @param {!Array.<string>} fieldNames
   * @return {!HeapSnapshotModel.HeapSnapshotModel.ComparatorConfig}
   */
  static createComparator(fieldNames) {
    return /** @type {!HeapSnapshotModel.HeapSnapshotModel.ComparatorConfig} */ (
        {fieldName1: fieldNames[0], ascending1: fieldNames[1], fieldName2: fieldNames[2], ascending2: fieldNames[3]});
  }

  /**
   * @return {!HeapSnapshotSortableDataGrid}
   */
  heapSnapshotDataGrid() {
    return this._dataGrid;
  }

  /**
   * @return {!ChildrenProvider}
   */
  createProvider() {
    throw new Error('Not implemented.');
  }

  /**
   * @return {?{snapshot:!HeapSnapshotProxy, snapshotNodeIndex:number}}
   */
  retainersDataSource() {
    return null;
  }

  /**
   * @return {!ChildrenProvider}
   */
  _provider() {
    if (!this._providerObject) {
      this._providerObject = this.createProvider();
    }
    return this._providerObject;
  }

  /**
   * @override
   * @param {string} columnId
   * @return {!HTMLElement}
   */
  createCell(columnId) {
    const cell = super.createCell(columnId);
    if (this._searchMatched) {
      cell.classList.add('highlight');
    }
    return cell;
  }

  /**
   * @override
   */
  collapse() {
    super.collapse();
    this._dataGrid.updateVisibleNodes(true);
  }

  /**
   * @override
   */
  expand() {
    super.expand();
    this._dataGrid.updateVisibleNodes(true);
  }

  dispose() {
    if (this._providerObject) {
      this._providerObject.dispose();
    }
    for (let node = this.children[0]; node; node = node.traverseNextNode(true, this, true)) {
      if (node.dispose) {
        node.dispose();
      }
    }
  }

  /**
   * @param {!SDK.HeapProfilerModel.HeapProfilerModel} heapProfilerModel
   * @param {string} objectGroupName
   * @return {!Promise<!SDK.RemoteObject.RemoteObject>}
   */
  queryObjectContent(heapProfilerModel, objectGroupName) {
  }

  /**
   * @param {!SDK.HeapProfilerModel.HeapProfilerModel} heapProfilerModel
   * @param {string} objectGroupName
   * @return {!Promise<?SDK.RemoteObject.RemoteObject>}
   */
  tryQueryObjectContent(heapProfilerModel, objectGroupName) {
  }

  /**
   * @param {!UI.ContextMenu.ContextMenu} contextMenu
   * @param {!DataDisplayDelegate} dataDisplayDelegate
   * @param {?SDK.HeapProfilerModel.HeapProfilerModel} heapProfilerModel
   */
  populateContextMenu(contextMenu, dataDisplayDelegate, heapProfilerModel) {
  }

  /**
   * @param {number} num
   * @return {string}
   */
  _toPercentString(num) {
    return num.toFixed(0) + '\xa0%';  // \xa0 is a non-breaking space.
  }

  /**
   * @param {number} distance
   * @return {string}
   */
  _toUIDistance(distance) {
    const baseSystemDistance = HeapSnapshotModel.HeapSnapshotModel.baseSystemDistance;
    return distance >= 0 && distance < baseSystemDistance ? Common.UIString.UIString('%d', distance) :
                                                            Common.UIString.UIString('\u2212');
  }

  /**
   * @return {!Array.<!DataGrid.DataGrid.DataGridNode>}
   */
  allChildren() {
    return this._dataGrid.allChildren(this);
  }

  /**
   * @param {number} index
   */
  removeChildByIndex(index) {
    this._dataGrid.removeChildByIndex(this, index);
  }

  /**
   * @param {number} nodePosition
   * @return {?DataGrid.DataGrid.DataGridNode}
   */
  childForPosition(nodePosition) {
    let indexOfFirstChildInRange = 0;
    for (let i = 0; i < this._retrievedChildrenRanges.length; i++) {
      const range = this._retrievedChildrenRanges[i];
      if (range.from <= nodePosition && nodePosition < range.to) {
        const childIndex = indexOfFirstChildInRange + nodePosition - range.from;
        return this.allChildren()[childIndex];
      }
      indexOfFirstChildInRange += range.to - range.from + 1;
    }
    return null;
  }

  /**
   * @param {string} columnId
   * @return {!HTMLElement}
   */
  _createValueCell(columnId) {
    const cell = /** @type {!HTMLElement} */ (UI.Fragment.html`<td class="numeric-column" />`);
    if (this.dataGrid.snapshot.totalSize !== 0) {
      const div = createElement('div');
      const valueSpan = UI.Fragment.html`<span>${this.data[columnId]}</span>`;
      div.appendChild(valueSpan);
      const percentColumn = columnId + '-percent';
      if (percentColumn in this.data) {
        const percentSpan = UI.Fragment.html`<span class="percent-column">${this.data[percentColumn]}</span>`;
        div.appendChild(percentSpan);
        div.classList.add('profile-multiple-values');
        UI.ARIAUtils.markAsHidden(valueSpan);
        UI.ARIAUtils.markAsHidden(percentSpan);
        this.setCellAccessibleName(ls`${this.data[columnId]}, ${this.data[percentColumn]}`, cell, columnId);
      }
      cell.appendChild(div);
    }
    return cell;
  }

  /**
   * @override
   */
  populate() {
    if (this._populated) {
      return;
    }
    this._populated = true;
    this._provider().sortAndRewind(this.comparator()).then(() => this._populateChildren());
  }

  /**
   * @return {!Promise}
   */
  expandWithoutPopulate() {
    // Make sure default populate won't take action.
    this._populated = true;
    this.expand();
    return this._provider().sortAndRewind(this.comparator());
  }

  /**
   * @param {?number=} fromPosition
   * @param {?number=} toPosition
   * @return {!Promise}
   */
  _populateChildren(fromPosition, toPosition) {
    let afterPopulate;
    const promise = new Promise(resolve => {
      afterPopulate = resolve;
    });
    fromPosition = fromPosition || 0;
    toPosition = toPosition || fromPosition + this._dataGrid.defaultPopulateCount();
    let firstNotSerializedPosition = fromPosition;
    serializeNextChunk.call(this);
    return promise;

    /**
     * @this {HeapSnapshotGridNode}
     */
    function serializeNextChunk() {
      if (firstNotSerializedPosition >= toPosition) {
        return;
      }
      const end = Math.min(firstNotSerializedPosition + this._dataGrid.defaultPopulateCount(), toPosition);
      this._provider().serializeItemsRange(firstNotSerializedPosition, end).then(childrenRetrieved.bind(this));
      firstNotSerializedPosition = end;
    }

    /**
     * @this {HeapSnapshotGridNode}
     */
    function insertRetrievedChild(item, insertionIndex) {
      if (this._savedChildren) {
        const hash = this._childHashForEntity(item);
        if (hash in this._savedChildren) {
          this._dataGrid.insertChild(this, this._savedChildren[hash], insertionIndex);
          return;
        }
      }
      this._dataGrid.insertChild(this, this._createChildNode(item), insertionIndex);
    }

    /**
     * @this {HeapSnapshotGridNode}
     */
    function insertShowMoreButton(from, to, insertionIndex) {
      const button = new DataGrid.ShowMoreDataGridNode.ShowMoreDataGridNode(
          this._populateChildren.bind(this), from, to, this._dataGrid.defaultPopulateCount());
      this._dataGrid.insertChild(this, button, insertionIndex);
    }

    /**
     * @param {!HeapSnapshotModel.HeapSnapshotModel.ItemsRange} itemsRange
     * @this {HeapSnapshotGridNode}
     */
    function childrenRetrieved(itemsRange) {
      let itemIndex = 0;
      let itemPosition = itemsRange.startPosition;
      const items = itemsRange.items;
      let insertionIndex = 0;

      if (!this._retrievedChildrenRanges.length) {
        if (itemsRange.startPosition > 0) {
          this._retrievedChildrenRanges.push({from: 0, to: 0});
          insertShowMoreButton.call(this, 0, itemsRange.startPosition, insertionIndex++);
        }
        this._retrievedChildrenRanges.push({from: itemsRange.startPosition, to: itemsRange.endPosition});
        for (let i = 0, l = items.length; i < l; ++i) {
          insertRetrievedChild.call(this, items[i], insertionIndex++);
        }
        if (itemsRange.endPosition < itemsRange.totalLength) {
          insertShowMoreButton.call(this, itemsRange.endPosition, itemsRange.totalLength, insertionIndex++);
        }
      } else {
        let rangeIndex = 0;
        let found = false;
        let range;
        while (rangeIndex < this._retrievedChildrenRanges.length) {
          range = this._retrievedChildrenRanges[rangeIndex];
          if (range.to >= itemPosition) {
            found = true;
            break;
          }
          insertionIndex += range.to - range.from;
          // Skip the button if there is one.
          if (range.to < itemsRange.totalLength) {
            insertionIndex += 1;
          }
          ++rangeIndex;
        }

        if (!found || itemsRange.startPosition < range.from) {
          // Update previous button.
          this.allChildren()[insertionIndex - 1].setEndPosition(itemsRange.startPosition);
          insertShowMoreButton.call(
              this, itemsRange.startPosition, found ? range.from : itemsRange.totalLength, insertionIndex);
          range = {from: itemsRange.startPosition, to: itemsRange.startPosition};
          if (!found) {
            rangeIndex = this._retrievedChildrenRanges.length;
          }
          this._retrievedChildrenRanges.splice(rangeIndex, 0, range);
        } else {
          insertionIndex += itemPosition - range.from;
        }
        // At this point insertionIndex is always an index before button or between nodes.
        // Also it is always true here that range.from <= itemPosition <= range.to

        // Stretch the range right bound to include all new items.
        while (range.to < itemsRange.endPosition) {
          // Skip already added nodes.
          const skipCount = range.to - itemPosition;
          insertionIndex += skipCount;
          itemIndex += skipCount;
          itemPosition = range.to;

          // We're at the position before button: ...<?node>x<button>
          const nextRange = this._retrievedChildrenRanges[rangeIndex + 1];
          let newEndOfRange = nextRange ? nextRange.from : itemsRange.totalLength;
          if (newEndOfRange > itemsRange.endPosition) {
            newEndOfRange = itemsRange.endPosition;
          }
          while (itemPosition < newEndOfRange) {
            insertRetrievedChild.call(this, items[itemIndex++], insertionIndex++);
            ++itemPosition;
          }

          // Merge with the next range.
          if (nextRange && newEndOfRange === nextRange.from) {
            range.to = nextRange.to;
            // Remove "show next" button if there is one.
            this.removeChildByIndex(insertionIndex);
            this._retrievedChildrenRanges.splice(rangeIndex + 1, 1);
          } else {
            range.to = newEndOfRange;
            // Remove or update next button.
            if (newEndOfRange === itemsRange.totalLength) {
              this.removeChildByIndex(insertionIndex);
            } else {
              this.allChildren()[insertionIndex].setStartPosition(itemsRange.endPosition);
            }
          }
        }
      }

      // TODO: fix this.
      this._instanceCount += items.length;
      if (firstNotSerializedPosition < toPosition) {
        serializeNextChunk.call(this);
        return;
      }

      if (this.expanded) {
        this._dataGrid.updateVisibleNodes(true);
      }
      afterPopulate();
      this.dispatchEventToListeners(HeapSnapshotGridNode.Events.PopulateComplete);
    }
  }

  _saveChildren() {
    this._savedChildren = null;
    const children = this.allChildren();
    for (let i = 0, l = children.length; i < l; ++i) {
      const child = children[i];
      if (!child.expanded) {
        continue;
      }
      if (!this._savedChildren) {
        this._savedChildren = {};
      }
      this._savedChildren[this._childHashForNode(child)] = child;
    }
  }

  async sort() {
    this._dataGrid.recursiveSortingEnter();

    await this._provider().sortAndRewind(this.comparator());

    this._saveChildren();
    this._dataGrid.removeAllChildren(this);
    this._retrievedChildrenRanges = [];
    const instanceCount = this._instanceCount;
    this._instanceCount = 0;

    await this._populateChildren(0, instanceCount);

    for (const child of this.allChildren()) {
      if (child.expanded) {
        child.sort();
      }
    }
    this._dataGrid.recursiveSortingLeave();
  }
}

/** @enum {symbol} */
HeapSnapshotGridNode.Events = {
  PopulateComplete: Symbol('PopulateComplete')
};

/**
 * @unrestricted
 */
export class HeapSnapshotGenericObjectNode extends HeapSnapshotGridNode {
  /**
   * @param {!HeapSnapshotSortableDataGrid} dataGrid
   * @param {!HeapSnapshotModel.HeapSnapshotModel.Node} node
   */
  constructor(dataGrid, node) {
    super(dataGrid, false);
    // node is null for DataGrid root nodes.
    if (!node) {
      return;
    }
    this._name = node.name;
    this._type = node.type;
    this._distance = node.distance;
    this._shallowSize = node.selfSize;
    this._retainedSize = node.retainedSize;
    this.snapshotNodeId = node.id;
    this.snapshotNodeIndex = node.nodeIndex;
    if (this._type === 'string') {
      this._reachableFromWindow = true;
    } else if (this._type === 'object' && this._name.startsWith('Window')) {
      this._name = this.shortenWindowURL(this._name, false);
      this._reachableFromWindow = true;
    } else if (node.canBeQueried) {
      this._reachableFromWindow = true;
    }
    if (node.detachedDOMTreeNode) {
      this.detachedDOMTreeNode = true;
    }

    const snapshot = dataGrid.snapshot;
    const shallowSizePercent = this._shallowSize / snapshot.totalSize * 100.0;
    const retainedSizePercent = this._retainedSize / snapshot.totalSize * 100.0;
    this.data = {
      'distance': this._toUIDistance(this._distance),
      'shallowSize': Number.withThousandsSeparator(this._shallowSize),
      'retainedSize': Number.withThousandsSeparator(this._retainedSize),
      'shallowSize-percent': this._toPercentString(shallowSizePercent),
      'retainedSize-percent': this._toPercentString(retainedSizePercent)
    };
  }

  /**
   * @override
   * @return {?{snapshot:!HeapSnapshotProxy, snapshotNodeIndex:number}}
   */
  retainersDataSource() {
    return {snapshot: this._dataGrid.snapshot, snapshotNodeIndex: this.snapshotNodeIndex};
  }

  /**
   * @override
   * @param {string} columnId
   * @return {!HTMLElement}
   */
  createCell(columnId) {
    const cell = columnId !== 'object' ? this._createValueCell(columnId) : this._createObjectCell();
    if (this._searchMatched) {
      cell.classList.add('highlight');
    }
    return cell;
  }

  /**
   * @return {!HTMLElement}
   */
  _createObjectCell() {
    let value = this._name;
    let valueStyle = 'object';
    switch (this._type) {
      case 'concatenated string':
      case 'string':
        value = `"${value}"`;
        valueStyle = 'string';
        break;
      case 'regexp':
        value = `/${value}/`;
        valueStyle = 'string';
        break;
      case 'closure':
        value = `${value}()`;
        valueStyle = 'function';
        break;
      case 'bigint':
        valueStyle = 'bigint';
        break;
      case 'number':
        valueStyle = 'number';
        break;
      case 'hidden':
        valueStyle = 'null';
        break;
      case 'array':
        value = value ? `${value}[]` : ls`(internal array)[]`;
        break;
    }
    return this._createObjectCellWithValue(valueStyle, value);
  }

  /**
   * @param {string} valueStyle
   * @param {string} value
   * @return {!HTMLElement}
   */
  _createObjectCellWithValue(valueStyle, value) {
    const fragment = UI.Fragment.Fragment.build`
        <td class="object-column disclosure">
          <div class="source-code event-properties" style="overflow: visible" $="container">
            <span class="value object-value-${valueStyle}">${value}</span>
            <span class="object-value-id">@${this.snapshotNodeId}</span>
          </div>
        </td>`;
    const div = fragment.$('container');
    this._prefixObjectCell(div);
    if (this._reachableFromWindow) {
      div.appendChild(UI.Fragment.html
                      `<span class="heap-object-tag" title="${ls`User object reachable from window`}">????</span>`);
    }
    if (this.detachedDOMTreeNode) {
      div.appendChild(UI.Fragment.html`<span class="heap-object-tag" title="${ls`Detached from DOM tree`}">???</span>`);
    }
    this._appendSourceLocation(div);
    const cell = /** @type {!HTMLElement} */ (fragment.element());
    if (this.depth) {
      cell.style.setProperty('padding-left', (this.depth * this.dataGrid.indentWidth) + 'px');
    }
    cell.heapSnapshotNode = this;
    return cell;
  }

  /**
   * @param {!Element} div
   */
  _prefixObjectCell(div) {
  }

  /**
   * @param {!Element} div
   */
  async _appendSourceLocation(div) {
    const linkContainer = UI.Fragment.html`<span class="heap-object-source-link" />`;
    div.appendChild(linkContainer);
    const link = await this._dataGrid.dataDisplayDelegate().linkifyObject(this.snapshotNodeIndex);
    if (link) {
      linkContainer.appendChild(link);
      this.linkElement = link;
    } else {
      linkContainer.remove();
    }
  }

  /**
   * @override
   * @param {!SDK.HeapProfilerModel.HeapProfilerModel} heapProfilerModel
   * @param {string} objectGroupName
   * @return {!Promise<!SDK.RemoteObject.RemoteObject>}
   */
  async queryObjectContent(heapProfilerModel, objectGroupName) {
    const remoteObject = await this.tryQueryObjectContent(heapProfilerModel, objectGroupName);
    return remoteObject ||
        heapProfilerModel.runtimeModel().createRemoteObjectFromPrimitiveValue(ls`Preview is not available`);
  }

  /**
   * @override
   * @param {!SDK.HeapProfilerModel.HeapProfilerModel} heapProfilerModel
   * @param {string} objectGroupName
   * @return {!Promise<?SDK.RemoteObject.RemoteObject>}
   */
  async tryQueryObjectContent(heapProfilerModel, objectGroupName) {
    if (this._type === 'string') {
      return heapProfilerModel.runtimeModel().createRemoteObjectFromPrimitiveValue(this._name);
    }
    return await heapProfilerModel.objectForSnapshotObjectId(String(this.snapshotNodeId), objectGroupName);
  }

  async updateHasChildren() {
    const isEmpty = await this._provider().isEmpty();
    this.setHasChildren(!isEmpty);
  }

  /**
   * @param {string} fullName
   * @param {boolean} hasObjectId
   * @return {string}
   */
  shortenWindowURL(fullName, hasObjectId) {
    const startPos = fullName.indexOf('/');
    const endPos = hasObjectId ? fullName.indexOf('@') : fullName.length;
    if (startPos === -1 || endPos === -1) {
      return fullName;
    }
    const fullURL = fullName.substring(startPos + 1, endPos).trimLeft();
    let url = Platform.StringUtilities.trimURL(fullURL);
    if (url.length > 40) {
      url = url.trimMiddle(40);
    }
    return fullName.substr(0, startPos + 2) + url + fullName.substr(endPos);
  }

  /**
   * @override
   * @param {!UI.ContextMenu.ContextMenu} contextMenu
   * @param {!DataDisplayDelegate} dataDisplayDelegate
   * @param {?SDK.HeapProfilerModel.HeapProfilerModel} heapProfilerModel
   */
  populateContextMenu(contextMenu, dataDisplayDelegate, heapProfilerModel) {
    contextMenu.revealSection().appendItem(ls`Reveal in Summary view`, () => {
      dataDisplayDelegate.showObject(String(this.snapshotNodeId), ls`Summary`);
    });

    if (this._referenceName) {
      for (const match of this._referenceName.matchAll(/\((?<objectName>[^@)]*) @(?<snapshotNodeId>\d+)\)/g)) {
        const {objectName, snapshotNodeId} = /** @type {!{objectName:string, snapshotNodeId:string}} */ (match.groups);
        contextMenu.revealSection().appendItem(
            ls`Reveal object '${objectName}' with id @${snapshotNodeId} in Summary view`, () => {
              dataDisplayDelegate.showObject(snapshotNodeId, ls`Summary`);
            });
      }
    }

    if (heapProfilerModel) {
      contextMenu.revealSection().appendItem(ls`Store as global variable`, async () => {
        const remoteObject = await this.tryQueryObjectContent(
            /** @type {!SDK.HeapProfilerModel.HeapProfilerModel} */ (heapProfilerModel), '');
        if (!remoteObject) {
          Common.Console.Console.instance().error(ls`Preview is not available`);
        } else {
          await SDK.ConsoleModel.ConsoleModel.instance().saveToTempVariable(
              UI.Context.Context.instance().flavor(SDK.RuntimeModel.ExecutionContext), remoteObject);
        }
      });
    }
  }
}

/**
 * @unrestricted
 */
export class HeapSnapshotObjectNode extends HeapSnapshotGenericObjectNode {
  /**
   * @param {!HeapSnapshotSortableDataGrid} dataGrid
   * @param {!HeapSnapshotProxy} snapshot
   * @param {!HeapSnapshotModel.HeapSnapshotModel.Edge} edge
   * @param {?HeapSnapshotObjectNode} parentObjectNode
   */
  constructor(dataGrid, snapshot, edge, parentObjectNode) {
    super(dataGrid, edge.node);
    this._referenceName = edge.name;
    this._referenceType = edge.type;
    this._edgeIndex = edge.edgeIndex;
    this._snapshot = snapshot;

    this._parentObjectNode = parentObjectNode;
    this._cycledWithAncestorGridNode = this._findAncestorWithSameSnapshotNodeId();
    if (!this._cycledWithAncestorGridNode) {
      this.updateHasChildren();
    }

    const data = this.data;
    data['count'] = '';
    data['addedCount'] = '';
    data['removedCount'] = '';
    data['countDelta'] = '';
    data['addedSize'] = '';
    data['removedSize'] = '';
    data['sizeDelta'] = '';
  }

  /**
   * @override
   * @return {?{snapshot:!HeapSnapshotProxy, snapshotNodeIndex:number}}
   */
  retainersDataSource() {
    return {snapshot: this._snapshot, snapshotNodeIndex: this.snapshotNodeIndex};
  }

  /**
   * @override
   * @return {!HeapSnapshotProviderProxy}
   */
  createProvider() {
    return this._snapshot.createEdgesProvider(this.snapshotNodeIndex);
  }

  _findAncestorWithSameSnapshotNodeId() {
    let ancestor = this._parentObjectNode;
    while (ancestor) {
      if (ancestor.snapshotNodeId === this.snapshotNodeId) {
        return ancestor;
      }
      ancestor = ancestor._parentObjectNode;
    }
    return null;
  }

  /**
   * @param {!HeapSnapshotModel.HeapSnapshotModel.Edge} item
   * @return {!HeapSnapshotObjectNode}
   */
  _createChildNode(item) {
    return new HeapSnapshotObjectNode(this._dataGrid, this._snapshot, item, this);
  }

  /**
   * @param {!HeapSnapshotModel.HeapSnapshotModel.Edge} edge
   * @return {number}
   */
  _childHashForEntity(edge) {
    return edge.edgeIndex;
  }

  /**
   * @param {!HeapSnapshotObjectNode} childNode
   * @return {number}
   */
  _childHashForNode(childNode) {
    return childNode._edgeIndex;
  }

  /**
   * @return {!HeapSnapshotModel.HeapSnapshotModel.ComparatorConfig}
   */
  comparator() {
    const sortAscending = this._dataGrid.isSortOrderAscending();
    const sortColumnId = this._dataGrid.sortColumnId();
    const sortFields = {
      object: ['!edgeName', sortAscending, 'retainedSize', false],
      count: ['!edgeName', true, 'retainedSize', false],
      shallowSize: ['selfSize', sortAscending, '!edgeName', true],
      retainedSize: ['retainedSize', sortAscending, '!edgeName', true],
      distance: ['distance', sortAscending, '_name', true]
    }[sortColumnId] ||
        ['!edgeName', true, 'retainedSize', false];
    return HeapSnapshotGridNode.createComparator(sortFields);
  }

  /**
   * @override
   * @param {!Element} div
   */
  _prefixObjectCell(div) {
    let name = this._referenceName || '(empty)';
    let nameClass = 'name';
    switch (this._referenceType) {
      case 'context':
        nameClass = 'object-value-number';
        break;
      case 'internal':
      case 'hidden':
      case 'weak':
        nameClass = 'object-value-null';
        break;
      case 'element':
        name = `[${name}]`;
        break;
    }
    if (this._cycledWithAncestorGridNode) {
      div.classList.add('cycled-ancessor-node');
    }
    div.prepend(UI.Fragment.html`<span class="property-name ${nameClass}">${name}</span>
                        <span class="grayed">${this._edgeNodeSeparator()}</span>`);
  }

  /**
   * @return {string}
   */
  _edgeNodeSeparator() {
    return '::';
  }
}

/**
 * @unrestricted
 */
export class HeapSnapshotRetainingObjectNode extends HeapSnapshotObjectNode {
  /**
   * @param {!HeapSnapshotSortableDataGrid} dataGrid
   * @param {!HeapSnapshotProxy} snapshot
   * @param {!HeapSnapshotModel.HeapSnapshotModel.Edge} edge
   * @param {?HeapSnapshotRetainingObjectNode} parentRetainingObjectNode
   */
  constructor(dataGrid, snapshot, edge, parentRetainingObjectNode) {
    super(dataGrid, snapshot, edge, parentRetainingObjectNode);
  }

  /**
   * @override
   * @return {!HeapSnapshotProviderProxy}
   */
  createProvider() {
    return this._snapshot.createRetainingEdgesProvider(this.snapshotNodeIndex);
  }

  /**
   * @override
   * @param {!HeapSnapshotModel.HeapSnapshotModel.Edge} item
   * @return {!HeapSnapshotRetainingObjectNode}
   */
  _createChildNode(item) {
    return new HeapSnapshotRetainingObjectNode(this._dataGrid, this._snapshot, item, this);
  }

  /**
   * @override
   * @return {string}
   */
  _edgeNodeSeparator() {
    return ls`in`;
  }

  /**
   * @override
   */
  expand() {
    this._expandRetainersChain(20);
  }

  /**
   * @param {number} maxExpandLevels
   */
  _expandRetainersChain(maxExpandLevels) {
    if (!this._populated) {
      this.once(HeapSnapshotGridNode.Events.PopulateComplete).then(() => this._expandRetainersChain(maxExpandLevels));
      this.populate();
      return;
    }
    super.expand();
    if (--maxExpandLevels > 0 && this.children.length > 0) {
      const retainer = this.children[0];
      if (retainer._distance > 1) {
        retainer._expandRetainersChain(maxExpandLevels);
        return;
      }
    }
    this._dataGrid.dispatchEventToListeners(HeapSnapshotRetainmentDataGrid.Events.ExpandRetainersComplete);
  }
}

/**
 * @unrestricted
 */
export class HeapSnapshotInstanceNode extends HeapSnapshotGenericObjectNode {
  /**
   * @param {!HeapSnapshotSortableDataGrid} dataGrid
   * @param {!HeapSnapshotProxy} snapshot
   * @param {!HeapSnapshotModel.HeapSnapshotModel.Node} node
   * @param {boolean} isDeletedNode
   */
  constructor(dataGrid, snapshot, node, isDeletedNode) {
    super(dataGrid, node);
    this._baseSnapshotOrSnapshot = snapshot;
    this._isDeletedNode = isDeletedNode;
    this.updateHasChildren();

    const data = this.data;
    data['count'] = '';
    data['countDelta'] = '';
    data['sizeDelta'] = '';
    if (this._isDeletedNode) {
      data['addedCount'] = '';
      data['addedSize'] = '';
      data['removedCount'] = '\u2022';
      data['removedSize'] = Number.withThousandsSeparator(this._shallowSize);
    } else {
      data['addedCount'] = '\u2022';
      data['addedSize'] = Number.withThousandsSeparator(this._shallowSize);
      data['removedCount'] = '';
      data['removedSize'] = '';
    }
  }

  /**
   * @override
   * @return {?{snapshot:!HeapSnapshotProxy, snapshotNodeIndex:number}}
   */
  retainersDataSource() {
    return {snapshot: this._baseSnapshotOrSnapshot, snapshotNodeIndex: this.snapshotNodeIndex};
  }

  /**
   * @override
   * @return {!HeapSnapshotProviderProxy}
   */
  createProvider() {
    return this._baseSnapshotOrSnapshot.createEdgesProvider(this.snapshotNodeIndex);
  }

  /**
   * @param {!HeapSnapshotModel.HeapSnapshotModel.Edge} item
   * @return {!HeapSnapshotObjectNode}
   */
  _createChildNode(item) {
    return new HeapSnapshotObjectNode(this._dataGrid, this._baseSnapshotOrSnapshot, item, null);
  }

  /**
   * @param {!HeapSnapshotModel.HeapSnapshotModel.Edge} edge
   * @return {number}
   */
  _childHashForEntity(edge) {
    return edge.edgeIndex;
  }

  /**
   * @param {!HeapSnapshotObjectNode} childNode
   * @return {number}
   */
  _childHashForNode(childNode) {
    return childNode._edgeIndex;
  }

  /**
   * @return {!HeapSnapshotModel.HeapSnapshotModel.ComparatorConfig}
   */
  comparator() {
    const sortAscending = this._dataGrid.isSortOrderAscending();
    const sortColumnId = this._dataGrid.sortColumnId();
    const sortFields = {
      object: ['!edgeName', sortAscending, 'retainedSize', false],
      distance: ['distance', sortAscending, 'retainedSize', false],
      count: ['!edgeName', true, 'retainedSize', false],
      addedSize: ['selfSize', sortAscending, '!edgeName', true],
      removedSize: ['selfSize', sortAscending, '!edgeName', true],
      shallowSize: ['selfSize', sortAscending, '!edgeName', true],
      retainedSize: ['retainedSize', sortAscending, '!edgeName', true]
    }[sortColumnId] ||
        ['!edgeName', true, 'retainedSize', false];
    return HeapSnapshotGridNode.createComparator(sortFields);
  }
}

/**
 * @unrestricted
 */
export class HeapSnapshotConstructorNode extends HeapSnapshotGridNode {
  /**
   * @param {!HeapSnapshotConstructorsDataGrid} dataGrid
   * @param {string} className
   * @param {!HeapSnapshotModel.HeapSnapshotModel.Aggregate} aggregate
   * @param {!HeapSnapshotModel.HeapSnapshotModel.NodeFilter} nodeFilter
   */
  constructor(dataGrid, className, aggregate, nodeFilter) {
    super(dataGrid, aggregate.count > 0);
    this._name = className;
    this._nodeFilter = nodeFilter;
    this._distance = aggregate.distance;
    this._count = aggregate.count;
    this._shallowSize = aggregate.self;
    this._retainedSize = aggregate.maxRet;

    const snapshot = dataGrid.snapshot;
    const retainedSizePercent = this._retainedSize / snapshot.totalSize * 100.0;
    const shallowSizePercent = this._shallowSize / snapshot.totalSize * 100.0;
    this.data = {
      'object': className,
      'count': Number.withThousandsSeparator(this._count),
      'distance': this._toUIDistance(this._distance),
      'shallowSize': Number.withThousandsSeparator(this._shallowSize),
      'retainedSize': Number.withThousandsSeparator(this._retainedSize),
      'shallowSize-percent': this._toPercentString(shallowSizePercent),
      'retainedSize-percent': this._toPercentString(retainedSizePercent)
    };
  }

  /**
   * @override
   * @return {!HeapSnapshotProviderProxy}
   */
  createProvider() {
    return this._dataGrid.snapshot.createNodesProviderForClass(this._name, this._nodeFilter);
  }

  /**
   * @param {number} snapshotObjectId
   * @return {!Promise<!Array<!HeapSnapshotGridNode>>}
   */
  async populateNodeBySnapshotObjectId(snapshotObjectId) {
    this._dataGrid.resetNameFilter();
    await this.expandWithoutPopulate();

    const nodePosition = await this._provider().nodePosition(snapshotObjectId);
    if (nodePosition === -1) {
      this.collapse();
      return [];
    }

    await this._populateChildren(nodePosition, null);

    const node = /** @type {?HeapSnapshotGridNode} */ (this.childForPosition(nodePosition));
    return node ? [this, node] : [];
  }

  /**
   * @param {string} filterValue
   * @return {boolean}
   */
  filteredOut(filterValue) {
    return this._name.toLowerCase().indexOf(filterValue) === -1;
  }

  /**
   * @override
   * @param {string} columnId
   * @return {!HTMLElement}
   */
  createCell(columnId) {
    const cell = columnId === 'object' ? super.createCell(columnId) : this._createValueCell(columnId);
    if (columnId === 'object' && this._count > 1) {
      cell.appendChild(UI.Fragment.html`<span class="objects-count">??${this._count}</span>`);
    }
    if (this._searchMatched) {
      cell.classList.add('highlight');
    }
    return cell;
  }

  /**
   * @param {!HeapSnapshotModel.HeapSnapshotModel.Node} item
   * @return {!HeapSnapshotInstanceNode}
   */
  _createChildNode(item) {
    return new HeapSnapshotInstanceNode(this._dataGrid, this._dataGrid.snapshot, item, false);
  }

  /**
   * @return {!HeapSnapshotModel.HeapSnapshotModel.ComparatorConfig}
   */
  comparator() {
    const sortAscending = this._dataGrid.isSortOrderAscending();
    const sortColumnId = this._dataGrid.sortColumnId();
    const sortFields = {
      object: ['name', sortAscending, 'id', true],
      distance: ['distance', sortAscending, 'retainedSize', false],
      shallowSize: ['selfSize', sortAscending, 'id', true],
      retainedSize: ['retainedSize', sortAscending, 'id', true]
    }[sortColumnId];
    return HeapSnapshotGridNode.createComparator(sortFields);
  }

  /**
   * @param {!HeapSnapshotModel.HeapSnapshotModel.Node} node
   * @return {number}
   */
  _childHashForEntity(node) {
    return node.id;
  }

  /**
   * @param {!HeapSnapshotInstanceNode} childNode
   * @return {number}
   */
  _childHashForNode(childNode) {
    return childNode.snapshotNodeId;
  }
}

/**
 * @implements {ChildrenProvider}
 * @unrestricted
 */
export class HeapSnapshotDiffNodesProvider {
  /**
   * @param {!HeapSnapshotProviderProxy} addedNodesProvider
   * @param {!Profiler.HeapSnapshotProviderProxy} deletedNodesProvider
   * @param {number} addedCount
   * @param {number} removedCount
   */
  constructor(addedNodesProvider, deletedNodesProvider, addedCount, removedCount) {
    this._addedNodesProvider = addedNodesProvider;
    this._deletedNodesProvider = deletedNodesProvider;
    this._addedCount = addedCount;
    this._removedCount = removedCount;
  }

  /**
   * @override
   */
  dispose() {
    this._addedNodesProvider.dispose();
    this._deletedNodesProvider.dispose();
  }

  /**
   * @override
   * @param {number} snapshotObjectId
   * @return {!Promise<number>}
   */
  nodePosition(snapshotObjectId) {
    throw new Error('Unreachable');
  }

  /**
   * @override
   * @return {!Promise<boolean>}
   */
  isEmpty() {
    return Promise.resolve(false);
  }

  /**
   * @override
   * @param {number} beginPosition
   * @param {number} endPosition
   * @return {!Promise<!HeapSnapshotModel.HeapSnapshotModel.ItemsRange>}
   */
  async serializeItemsRange(beginPosition, endPosition) {
    let itemsRange;
    let addedItems;
    if (beginPosition < this._addedCount) {
      itemsRange = await this._addedNodesProvider.serializeItemsRange(beginPosition, endPosition);

      for (const item of itemsRange.items) {
        item.isAddedNotRemoved = true;
      }

      if (itemsRange.endPosition >= endPosition) {
        itemsRange.totalLength = this._addedCount + this._removedCount;
        return itemsRange;
      }

      addedItems = itemsRange;
      itemsRange = await this._deletedNodesProvider.serializeItemsRange(0, endPosition - itemsRange.endPosition);
    } else {
      addedItems = new HeapSnapshotModel.HeapSnapshotModel.ItemsRange(0, 0, 0, []);
      itemsRange = await this._deletedNodesProvider.serializeItemsRange(
          beginPosition - this._addedCount, endPosition - this._addedCount);
    }

    if (!addedItems.items.length) {
      addedItems.startPosition = this._addedCount + itemsRange.startPosition;
    }
    for (const item of itemsRange.items) {
      item.isAddedNotRemoved = false;
    }
    addedItems.items.push(...itemsRange.items);
    addedItems.endPosition = this._addedCount + itemsRange.endPosition;
    addedItems.totalLength = this._addedCount + this._removedCount;
    return addedItems;
  }

  /**
   * @override
   * @param {!HeapSnapshotModel.HeapSnapshotModel.ComparatorConfig} comparator
   * @return {!Promise}
   */
  async sortAndRewind(comparator) {
    await this._addedNodesProvider.sortAndRewind(comparator);
    await this._deletedNodesProvider.sortAndRewind(comparator);
  }
}

/**
 * @unrestricted
 */
export class HeapSnapshotDiffNode extends HeapSnapshotGridNode {
  /**
   * @param {!HeapSnapshotDiffDataGrid} dataGrid
   * @param {string} className
   * @param {!HeapSnapshotModel.HeapSnapshotModel.DiffForClass} diffForClass
   */
  constructor(dataGrid, className, diffForClass) {
    super(dataGrid, true);
    this._name = className;
    this._addedCount = diffForClass.addedCount;
    this._removedCount = diffForClass.removedCount;
    this._countDelta = diffForClass.countDelta;
    this._addedSize = diffForClass.addedSize;
    this._removedSize = diffForClass.removedSize;
    this._sizeDelta = diffForClass.sizeDelta;
    this._deletedIndexes = diffForClass.deletedIndexes;
    this.data = {
      'object': className,
      'addedCount': Number.withThousandsSeparator(this._addedCount),
      'removedCount': Number.withThousandsSeparator(this._removedCount),
      'countDelta': this._signForDelta(this._countDelta) + Number.withThousandsSeparator(Math.abs(this._countDelta)),
      'addedSize': Number.withThousandsSeparator(this._addedSize),
      'removedSize': Number.withThousandsSeparator(this._removedSize),
      'sizeDelta': this._signForDelta(this._sizeDelta) + Number.withThousandsSeparator(Math.abs(this._sizeDelta))
    };
  }

  /**
   * @override
   * @return {!HeapSnapshotDiffNodesProvider}
   */
  createProvider() {
    const tree = this._dataGrid;
    return new HeapSnapshotDiffNodesProvider(
        tree.snapshot.createAddedNodesProvider(tree.baseSnapshot.uid, this._name),
        tree.baseSnapshot.createDeletedNodesProvider(this._deletedIndexes), this._addedCount, this._removedCount);
  }

  /**
   * @override
   * @param {string} columnId
   * @return {!HTMLElement}
   */
  createCell(columnId) {
    const cell = super.createCell(columnId);
    if (columnId !== 'object') {
      cell.classList.add('numeric-column');
    }
    return cell;
  }

  /**
   * @param {!HeapSnapshotModel.HeapSnapshotModel.Node} item
   * @return {!HeapSnapshotInstanceNode}
   */
  _createChildNode(item) {
    if (item.isAddedNotRemoved) {
      return new HeapSnapshotInstanceNode(this._dataGrid, this._dataGrid.snapshot, item, false);
    }
    return new HeapSnapshotInstanceNode(this._dataGrid, this._dataGrid.baseSnapshot, item, true);
  }

  /**
   * @param {!HeapSnapshotModel.HeapSnapshotModel.Node} node
   * @return {number}
   */
  _childHashForEntity(node) {
    return node.id;
  }

  /**
   * @param {!HeapSnapshotInstanceNode} childNode
   * @return {number}
   */
  _childHashForNode(childNode) {
    return childNode.snapshotNodeId;
  }

  /**
   * @return {!HeapSnapshotModel.HeapSnapshotModel.ComparatorConfig}
   */
  comparator() {
    const sortAscending = this._dataGrid.isSortOrderAscending();
    const sortColumnId = this._dataGrid.sortColumnId();
    const sortFields = {
      object: ['name', sortAscending, 'id', true],
      addedCount: ['name', true, 'id', true],
      removedCount: ['name', true, 'id', true],
      countDelta: ['name', true, 'id', true],
      addedSize: ['selfSize', sortAscending, 'id', true],
      removedSize: ['selfSize', sortAscending, 'id', true],
      sizeDelta: ['selfSize', sortAscending, 'id', true]
    }[sortColumnId];
    return HeapSnapshotGridNode.createComparator(sortFields);
  }

  /**
   * @param {string} filterValue
   * @return {boolean}
   */
  filteredOut(filterValue) {
    return this._name.toLowerCase().indexOf(filterValue) === -1;
  }

  _signForDelta(delta) {
    if (delta === 0) {
      return '';
    }
    if (delta > 0) {
      return '+';
    }
    return '\u2212';  // Math minus sign, same width as plus.
  }
}

/**
 * @unrestricted
 */
export class AllocationGridNode extends HeapSnapshotGridNode {
  /**
   * @param {!AllocationDataGrid} dataGrid
   * @param {!HeapSnapshotModel.HeapSnapshotModel.SerializedAllocationNode} data
   */
  constructor(dataGrid, data) {
    super(dataGrid, data.hasChildren);
    this._populated = false;
    this._allocationNode = data;
    this.data = {
      'liveCount': Number.withThousandsSeparator(data.liveCount),
      'count': Number.withThousandsSeparator(data.count),
      'liveSize': Number.withThousandsSeparator(data.liveSize),
      'size': Number.withThousandsSeparator(data.size),
      'name': data.name
    };
  }

  /**
   * @override
   */
  populate() {
    if (this._populated) {
      return;
    }
    this._doPopulate();
  }

  async _doPopulate() {
    this._populated = true;

    const callers = await this._dataGrid.snapshot.allocationNodeCallers(this._allocationNode.id);

    const callersChain = callers.nodesWithSingleCaller;
    let parentNode = this;
    const dataGrid = /** @type {!AllocationDataGrid} */ (this._dataGrid);
    for (const caller of callersChain) {
      const child = new AllocationGridNode(dataGrid, caller);
      dataGrid.appendNode(parentNode, child);
      parentNode = child;
      parentNode._populated = true;
      if (this.expanded) {
        parentNode.expand();
      }
    }

    const callersBranch = callers.branchingCallers;
    callersBranch.sort(this._dataGrid._createComparator());
    for (const caller of callersBranch) {
      dataGrid.appendNode(parentNode, new AllocationGridNode(dataGrid, caller));
    }
    dataGrid.updateVisibleNodes(true);
  }

  /**
   * @override
   */
  expand() {
    super.expand();
    if (this.children.length === 1) {
      this.children[0].expand();
    }
  }

  /**
   * @override
   * @param {string} columnId
   * @return {!HTMLElement}
   */
  createCell(columnId) {
    if (columnId !== 'name') {
      return this._createValueCell(columnId);
    }

    const cell = super.createCell(columnId);
    const allocationNode = this._allocationNode;
    const heapProfilerModel = this._dataGrid.heapProfilerModel();
    if (allocationNode.scriptId) {
      const linkifier = this._dataGrid._linkifier;
      const urlElement = linkifier.linkifyScriptLocation(
          heapProfilerModel ? heapProfilerModel.target() : null, String(allocationNode.scriptId),
          allocationNode.scriptName, allocationNode.line - 1, allocationNode.column - 1, 'profile-node-file');
      urlElement.style.maxWidth = '75%';
      cell.insertBefore(urlElement, cell.firstChild);
    }
    return cell;
  }

  /**
   * @return {number}
   */
  allocationNodeId() {
    return this._allocationNode.id;
  }
}
