#include <iostream>
#include "Graph.h"

using namespace std;

// Função auxiliar para imprimir o fluxo máximo e as arestas usadas
void printFlow(Graph<string>& g, const string& source, const string& sink) {
    double maxFlow = g.edmondsKarp(source, sink);
    cout << "Max flow de " << source << " para " << sink << ": " << maxFlow << endl;

    auto used = g.getUsedEdges();
    for (auto e : used) {
        cout << e->getOrig()->getInfo() << " -> " << e->getDest()->getInfo()
             << " | flow: " << e->getFlow() << endl;
    }
    cout << "-----------------------" << endl;
}

int main() {

    cout << "Exemplo 1: Grafo normal" << endl;
    Graph<string> g1;
    g1.addVertex("S"); g1.addVertex("A"); g1.addVertex("B"); g1.addVertex("T");
    g1.addBidirectionalEdge("S", "A", 3);
    g1.addBidirectionalEdge("S", "B", 2);
    g1.addBidirectionalEdge("A", "B", 1);
    g1.addBidirectionalEdge("A", "T", 2);
    g1.addBidirectionalEdge("B", "T", 3);

    printFlow(g1, "S", "T");

    cout << "Exemplo 2: Risco = 1 (uma falha bloqueia o fluxo)" << endl;
    Graph<string> g2;
    g2.addVertex("S"); g2.addVertex("A"); g2.addVertex("B"); g2.addVertex("T");
    g2.addBidirectionalEdge("S", "A", 2);
    g2.addBidirectionalEdge("A", "T", 2);  // único caminho A->T
    g2.addBidirectionalEdge("S", "B", 1);
    g2.addBidirectionalEdge("B", "T", 1);  // único caminho B->T

    printFlow(g2, "S", "T");

    cout << "Exemplo 3: Fluxo com múltiplos caminhos e ciclos" << endl;
    Graph<string> g3;
    g3.addVertex("S"); g3.addVertex("A"); g3.addVertex("B"); g3.addVertex("C"); g3.addVertex("T");
    g3.addBidirectionalEdge("S", "A", 4);
    g3.addBidirectionalEdge("S", "B", 2);
    g3.addBidirectionalEdge("A", "C", 3);
    g3.addBidirectionalEdge("B", "C", 1);
    g3.addBidirectionalEdge("C", "A", 1); // ciclo
    g3.addBidirectionalEdge("C", "T", 5);

    printFlow(g3, "S", "T");

    return 0;
}