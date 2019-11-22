'use strict';

/* jerry heap snapshot to V8 heap snapshot converter */

const fs = require('fs');

class SnapshotConverter {
  constructor(jerrySnapshotPath, outputPath) {
    const inputData = fs.readFileSync(jerrySnapshotPath, 'utf8');
    this.jerrySnapshot = JSON.parse(inputData);
    this.outputPath = outputPath;
    this.strings = ['<dummy>'];
    this.stringMap = new Map();
    this.nodes = [];
    this.nodeMap = new Map();
    this.edges = [];
    this.edgeMap = new Map();
    this.nodesOutput = [];
    this.edgesOutput = [];
  }

  addString(string) {
    if (!this.stringMap.has(string.id)) {
      this.stringMap.set(string.id, this.strings.length);
      this.strings.push(string.chars);
    }
  }

  addNode(node) {
    if (!this.nodeMap.has(node.id)) {
      this.nodeMap.set(node.id, this.nodes.length);
      this.edgeMap.set(node.id, []);
      this.nodes.push(node);
    }
  }

  addEdge(edge) {
    // any edge must appear after it's from node,
    // so this.edgeMap should has edge.from
    this.edgeMap.get(edge.from).push(edge);
  }

  parse() {
    const elements = this.jerrySnapshot.elements;
    for (let i = 0; i < elements.length; i++) {
      const element = elements[i];
      if (element.type === 'string') {
        this.addString(element);
      } else if (element.type === 'node') {
        this.addNode(element);
      } else if (element.type === 'edge') {
        this.addEdge(element);
      }
    }
  }

  genNodeEdges(edges) {
    for (let i = 0; i < edges.length; i++) {
      const edge = edges[i];
      this.edgesOutput.push(edge.edge_type);
      const name = this.stringMap.get(edge.name);
      this.edgesOutput.push(name);
      // v8 node items is a flattern array,
      // so to_node is to_node_id * node_size
      this.edgesOutput.push(this.nodeMap.get(edge.to) * 6);
    }
  }

  genNodes() {
    for (const [node_id, index] of this.nodeMap) {
      const node = this.nodes[index];
      this.nodesOutput.push(node.node_type);
      this.nodesOutput.push(this.stringMap.get(node.name));
      this.nodesOutput.push(node.id);
      this.nodesOutput.push(node.size);
      this.nodesOutput.push(this.edgeMap.get(node_id).length);
      this.nodesOutput.push(0); // trace node id

      this.genNodeEdges(this.edgeMap.get(node_id));
    }
  }

  write() {
    const outputJSON = require('./snapshot-tmpl.json');
    outputJSON.snapshot.node_count = this.nodesOutput.length / 6,
    outputJSON.snapshot.edge_count = this.edgesOutput.length / 3,
    outputJSON.nodes = this.nodesOutput;
    outputJSON.edges = this.edgesOutput;
    outputJSON.strings = this.strings;

    fs.writeFileSync(this.outputPath, JSON.stringify(outputJSON, null, '  '));
  }
}

const args = process.argv.splice(2);
const converter = new SnapshotConverter(args[0], args[1]);
converter.parse();
converter.genNodes();
converter.write();
