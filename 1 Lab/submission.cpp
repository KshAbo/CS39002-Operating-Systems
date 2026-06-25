#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <bits/stdc++.h>
using namespace std;

int main(int argc, char const *argv[])
{
    int64_t c = 0;
    string path = "./data/graph.txt";
    ifstream input_file(path);

    // Construct graph data structure from reading the input file
    if (!input_file.is_open())
    {
        cout << "Error opening file! Exiting..." << endl;
        exit(1);
    }

    string line;
    getline(input_file, line);

    int64_t n = stoi(line);
    vector<vector<int64_t>> graph(n + 1);

    while (getline(input_file, line))
    {
        istringstream word(line);
        string node;
        word >> node;
        int64_t u = stoi(node);

        string _;
        word >> _;

        string node_;
        while (word >> node_)
        {
            int64_t v = stoi(node_);
            graph[u].push_back(v);
        }
    }

    vector<bool> vis(n + 1, false);

    if (argc == 1)
    {
        vis[1] = true;
        cout << "*** Process " << getpid() << ":" << endl;

        int status;
        int64_t child_id = fork();

        if (child_id == 0)
        {
            char *args[] = {(char *)"./program", (char *)"1", NULL};
            cout << "*** Process " << getpid() << ": " << 1 << endl;
            execv("./program", args);
        }

        waitpid(child_id, &status, 0);

        if (status == 0)
        {
            exit(0);
        }
        else
        {
            cout << "No Hamiltonian cycle found" << endl;
            exit(1);
        }
    }
    else
    {
        c = argc;

        char **args = (char **)malloc(sizeof(char *) * (argc + 2));
        args[0] = strdup("./program");

        for (int i = 1; i < argc; i++)
        {
            args[i] = strdup(argv[i]);
            vis[stoi(argv[i])] = true;
        }

        int64_t end_node = stoi(argv[argc - 1]);

        // All nodes visited, check for the first node as neighbour now
        if (argc == n + 1)
        {
            for (int v : graph[end_node])
            {
                if (v == 1)
                {
                    cout << endl;
                    cout << "Hamiltonian cycle found: ";
                    for (int i = 1; i < argc; i++)
                        cout << argv[i] << " ";
                    cout << "1" << endl;

                    exit(0);
                }
            }
            exit(1);
        }

        for (const int64_t &nei : graph[end_node])
        {
            if (vis[nei])
                continue;

            string str = to_string(nei);
            args[argc] = strdup(str.c_str());
            args[argc + 1] = NULL;

            cout << "*** Process " << getpid() << ": ";
            for (int i = 1; i <= argc; i++)
                cout << args[i] << " ";
            cout << endl;

            int64_t child_id = fork();
            int status;

            if (child_id == 0)
            {
                execv("./program", args);
            }

            waitpid(child_id, &status, 0);

            if (status == 0)
            {
                exit(0);
            }
        }

        exit(1);
    }

    return 0;
}
