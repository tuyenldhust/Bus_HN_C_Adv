#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libfdr/jrb.h"
#include "libfdr/dllist.h"

#define INFINITIVE_VALUE 10000000
#define MAXLEN 150
#define MAXFIELDS 100
#define talloc(ty, sz) (ty *)malloc(sz * sizeof(ty))
#define SIZE_MAX 100

typedef struct _line_bus
{
	char tuyen_bus[MAXLEN];			// Tên tuyến bus
	int id_diem_bus[MAXFIELDS]; // Danh sách id của điểm bus;
	int num_diem_bus;						// Số điểm bus trên một dòng khi đọc dữ liệu từ file
} * LineBus;

typedef struct
{
	JRB edges;		// Ánh xạ ID sang tên
	JRB vertices; // Lưu trữ cạnh
	JRB name2ID;	// Ánh xạ từ tên sang ID thuận tiện cho việc tìm kiếm
} Graph;

typedef struct
{
	int start;
	int end;
} StartEnd;

Graph createGraph();
void addVertex(Graph graph, int id, char *name);
char *getVertex(Graph graph, int id);
void addEdge(Graph graph, int v1, int v2, char *tuyen_bus);
JRB getEdgeValue(Graph graph, int v1, int v2);
int indegree(Graph graph, int v, int *output);
int outdegree(Graph graph, int v, int *output);
void dropGraph(Graph graph);
double shortestPath(Graph graph, int s, int t, int numV, int *path, int *length);

// Hàm bỏ kí tự trắng ở đầu và cuối của chuỗi s
void trim(char *s)
{
	char *p = s;
	int l = strlen(p);

	while (isspace(p[l - 1])) // isspace la kiem tra xem mot ki tu co phai la khoang trang hay khong
		p[--l] = 0;
	while (*p && isspace(*p))
		++p, --l;

	memmove(s, p, l + 1); // gan giong ham memcpy()
}

// Hàm đọc file
void readFile(Graph g, char *filename)
{
	int ID = 0;																			// ID bắt đâu băng 0
	char road[1500];																// Dùng để lưu trữ chuỗi sau dấu : trong file
	LineBus line_bus = talloc(struct _line_bus, 1); // Cấp phát động
	FILE *f = fopen(filename, "r");									// Mở file
	JRB Node;

	if (f == NULL)
	{
		free(line_bus);
		return NULL;
	}
	else
	{
		while (1)
		{
			line_bus->num_diem_bus = 0;
			if (feof(f))
			{
				break;
			}

			fscanf(f, "%[^:]:%[^\n]%*c", line_bus->tuyen_bus, road);
			trim(road);

			// printf("%s: %s\n", line_bus->tuyen_bus, line_bus->id_diem_bus);

			char *token;
			/* lay token dau tien */
			token = strtok(road, "-");
			/* duyet qua cac token con lai */
			while (token != NULL)
			{
				trim(token);

				// Kiểm tra xem có đã tồn tại tên điểm bus trong JRB name2ID hay chưa
				// Nếu chưa tồn tại thì thêm vào
				if (jrb_find_str(g.name2ID, token) == NULL)
				{
					addVertex(g, ID, token);																	// Thực hiện ánh xạ ID -> tên điểm bus
					jrb_insert_str(g.name2ID, strdup(token), new_jval_i(ID)); // Thực hiện ánh xạ tên điểm bus -> ID để thuận tiện cho việc tìm kiếm ID của điểm bus nhờ vào tên của điểm bus
					ID++;
				}

				// Nếu đã tồn tại thì lấy ID của nó để lưu vào mảng id_diem_bus
				if ((Node = jrb_find_str(g.name2ID, token)) != NULL)
				{

					line_bus->id_diem_bus[line_bus->num_diem_bus++] = jval_i(Node->val);
				}
				token = strtok(NULL, "-");
			}

			for (int i = 0; i < line_bus->num_diem_bus - 1; i++)
			{
				// Thêm cạnh
				// Giá trị (Value) của đỉnh kề trong cây JRB không phải là trọng số nữa mà là một cây JRB lưu tên các tuyến đường đi qua 2 đỉnh
				// Vì đề bài không nói gì về trọng số của đường đi nên ta sẽ coi nó bằng 1 nhé.
				addEdge(g, line_bus->id_diem_bus[i], line_bus->id_diem_bus[i + 1], line_bus->tuyen_bus);
			}
		}
	}

	free(line_bus);
}

int main()
{
	StartEnd st[SIZE_MAX]; // Dùng để lưu start, end của 1 chuyến đi sẽ được sử dụng cho thuật toán xây dựng cách di chuyển ở phía dưới 
	int length, path[100], s, t, index = 0;
	char startBus[MAXLEN], endBus[MAXLEN];
	double w;
	JRB cur, list_tuyen_bus = NULL, list_tuyen_bus2 = NULL, guide;

	Graph g = createGraph();
	readFile(g, "xe_buyt.txt");

	printf("Start Bus: ");
	scanf("%[^\n]%*c", startBus);
	printf("End Bus: ");
	scanf("%[^\n]%*c", endBus);

	s = jval_i(jrb_val(jrb_find_str(g.name2ID, startBus)));
	t = jval_i(jrb_val(jrb_find_str(g.name2ID, endBus)));

	w = shortestPath(g, s, t, jval_i(jrb_last(g.vertices)->key), path, &length); // Short test path
	if (w == INFINITIVE_VALUE)
	{
		printf("No path from %s to %s\n", startBus, endBus);
	}
	else
	{
		printf("Path from %s to %s (with total distance %f)\n", startBus, endBus, w);
		for (int i = 0; i < length; i++)
			printf(" => %s", getVertex(g, path[i]));

		printf("\n\n");
		list_tuyen_bus2 = make_jrb(); // Dùng để lưu tạm các chuyến có cùng tuyến với chuyến đã xét trước đó
		guide = make_jrb(); // Dùng để lưu các bước đi. Khóa sẽ là bước thứ bao nhiêu đó và giá trị là một cây JRB để lưu danh sách tuyến

		// Thuật toán bên dưới xây dựng cách di chuyển
		for (int i = 0; i < length - 1; i++)
		{
			// Lấy giá trị của cạnh. Ở đây giá trị của cạnh được coi là tên các tuyến bus
			// Ta sẽ lấy giá trị cạnh nối đỉnh i và đỉnh i + 1
			cur = getEdgeValue(g, path[i], path[i + 1]);

			if (list_tuyen_bus == NULL)
			{
				list_tuyen_bus = cur;

				// Thêm chỉ dẫn
				jrb_insert_int(guide, index, new_jval_v(list_tuyen_bus));
				// Đặt chỉ dẫn đầu tiên (index = 0) với start và end là chỉ số trong mảng path để tí nữa ta in ra tên cho dễ
				st[index].start = i;
				st[index].end = i + 1;
			}
			else
			{
				JRB ptr;
				int count = 0; // Đếm số tuyến bus có trùng nhau hay không? Nếu trùng thì không phải chuyển bus, nếu không thì phải chuyển bus
				// Phần này sẽ tìm xem giá trị cạnh của hai đỉnh hiện tại có tuyến bus nào trong giá trị cạnh của hai đỉnh của lần lặp trước hay không?
				// Nếu có sẽ dùng chuyến đó để đi tiếp
				// Nếu không sẽ buộc phải chuyển bus
				// Biến count dùng để đếm số chuyến bus trùng
				// count = 0 tức là chuyển bus
				// count != 0 sẽ dùng các chuyến bus trùng đó để đi tiếp 
				jrb_traverse(ptr, cur)
				{
					JRB tmp = jrb_find_str(list_tuyen_bus, jval_s(ptr->key));
					if (tmp != NULL) // Tìm thấy một chuyến bus trùng
					{
						count++; // Tăng count lên 1

						// Thêm tên chuyến bus trùng vào cây JRB tạm thời list_tuyen_bus2
						jrb_insert_str(list_tuyen_bus2, jval_s(tmp->key), new_jval_i(1));
					}
				}

				if (count != 0)
				{
					// Update lại chỉ dẫn ( dòng 202 -> 204)
					JRB node = jrb_find_int(guide, index);
					jrb_delete_node(node);
					jrb_insert_int(guide, index, new_jval_v(list_tuyen_bus2));

					list_tuyen_bus = list_tuyen_bus2;
					list_tuyen_bus2 = make_jrb();

					st[index].end = i + 1; // Cho điểm kết thúc của chỉ dẫn thứ "index" là i + 1. Vì ta lấy giá trị cạnh hai dỉnh i và i + 1 
				}
				else
				{
					index++;// Tẳng chỉ dẫn lên hay nói cách khác là chuyển sang bước tiếp theo( chuyển bus)
					list_tuyen_bus = cur;

					// Thêm chỉ dẫn mới
					jrb_insert_int(guide, index, new_jval_v(list_tuyen_bus));

					// Đặt start, end của chỉ dẫn mới và end của chỉ dẫn trước đó
					st[index].start = i;
					st[index].end = i + 1;
					st[index - 1].end = i;
				}
			}
		}

		printf("-----> Cach di <-----\n");

		JRB ptr, list_tuyen, node;

		// Lặp qua từng bước
		jrb_traverse(ptr, guide)
		{
			index = jval_i(ptr->key);
			printf("B%d:\nChuyen: ", index + 1);

			// Lấy giá trị của các bưỚc(Là cây JRB lưu danh sách các tuyến bus để chỉ dẫn ngưƠi dùng)
			list_tuyen = jval_v(jrb_val(ptr));

			//Lấy tên các chuyến bus
			jrb_traverse(node, list_tuyen)
			{
				printf("%s  ", jval_s(node->key));
			}
			printf("\n");

			// In ra các điểm bus với index là bước thứ bao nhiêu đó trong cách đi
			for (int k = st[index].start; k <= st[index].end; k++)
			{
				printf("=> %s ", getVertex(g, path[k]));
			}
			printf("\n-------------------\n");
		}
	}


	// Free guide
	JRB ptr;
	jrb_traverse(ptr, guide){
		JRB val = jval_v(jrb_val(ptr));
		jrb_free_tree(val);
	}

	jrb_free_tree(guide);

	dropGraph(g);
}

Graph createGraph()
{
	Graph g;
	g.edges = make_jrb();
	g.vertices = make_jrb();
	g.name2ID = make_jrb();
	return g;
}

void addVertex(Graph g, int id, char *name)
{
	JRB node = jrb_find_int(g.vertices, id);
	if (node == NULL) // only add new vertex
		jrb_insert_int(g.vertices, id, new_jval_s(strdup(name)));
}

char *getVertex(Graph g, int id)
{
	JRB node = jrb_find_int(g.vertices, id);
	if (node == NULL)
		return NULL;
	else
		return jval_s(node->val);
}

void addEdge(Graph graph, int v1, int v2, char *tuyen_bus)
{
	// Theo như những gì đã học thì giá trị của một nút trong g.edges là một cây JRB dùng để lưu trử các đỉnh kề vớI đỉnh hiện tại
	// Và giá trị của mỗi nút trong danh sách đỉnh kề là trọng số
	// Ở đây tôi sẽ thay đổi một chút
	// Tôi sẽ dùng cây JRB khác để làm trọng số
	// Trong đó cây JRB này sẽ lưu các tuyến bus đi qua đỉnh hiện tại và đỉnh kề


	JRB node, tree, value, tree2;
	char chuyen[30];

	value = getEdgeValue(graph, v1, v2);
	if (value == NULL)
	{
		node = jrb_find_int(graph.edges, v1);
		if (node == NULL)
		{
			tree = make_jrb();
			jrb_insert_int(graph.edges, v1, new_jval_v(tree));
		}
		else
		{
			tree = (JRB)jval_v(node->val);
		}

		sprintf(chuyen, "%s(Chuyen Di)", tuyen_bus);
		tree2 = make_jrb();		// tree2 dùng để lưu các tuyến đường có thể đi từ v1 đến v2
		jrb_insert_str(tree2, strdup(chuyen), new_jval_i(1)); // Số 1 không quan trọng để số nào cũng đc nhé :v
		jrb_insert_int(tree, v2, new_jval_v(tree2));
	}
	else
	{
		sprintf(chuyen, "%s(Chuyen Di)", tuyen_bus);
		node = jrb_find_str(value, chuyen);
		if (node == NULL)
		{
			jrb_insert_str(value, strdup(chuyen), new_jval_i(1));
		}
	}

	value = getEdgeValue(graph, v2, v1);
	if (value == NULL)
	{
		node = jrb_find_int(graph.edges, v2);
		if (node == NULL)
		{
			tree = make_jrb();
			jrb_insert_int(graph.edges, v2, new_jval_v(tree));
		}
		else
		{
			tree = (JRB)jval_v(node->val);
		}

		sprintf(chuyen, "%s(Chuyen Ve)", tuyen_bus);
		tree2 = make_jrb();		// tree2 dùng để lưu các tuyến đường có thể đi từ v1 đến v2
		jrb_insert_str(tree2, strdup(chuyen), new_jval_i(1)); // Số 1 không quan trọng để số nào cũng đc nhé :v
		jrb_insert_int(tree, v1, new_jval_v(tree2));
	}
	else
	{
		sprintf(chuyen, "%s(Chuyen Ve)", tuyen_bus);
		node = jrb_find_str(value, chuyen);
		if (node == NULL)
		{
			jrb_insert_str(value, strdup(chuyen), new_jval_i(1));
		}
	}
}

JRB getEdgeValue(Graph graph, int v1, int v2)
{
	JRB node, tree;
	node = jrb_find_int(graph.edges, v1);
	if (node == NULL)
		return NULL;
	tree = (JRB)jval_v(node->val);
	node = jrb_find_int(tree, v2);
	if (node == NULL)
		return NULL;
	else
		return jval_v(node->val);
}

int indegree(Graph graph, int v, int *output)
{
	JRB tree, node;
	int total = 0;
	jrb_traverse(node, graph.edges)
	{
		tree = (JRB)jval_v(node->val);
		if (jrb_find_int(tree, v))
		{
			output[total] = jval_i(node->key);
			total++;
		}
	}
	return total;
}

int outdegree(Graph graph, int v, int *output)
{
	JRB tree, node;
	int total;
	node = jrb_find_int(graph.edges, v);
	if (node == NULL)
		return 0;
	tree = (JRB)jval_v(node->val);
	total = 0;
	jrb_traverse(node, tree)
	{
		output[total] = jval_i(node->key);
		total++;
	}
	return total;
}

double shortestPath(Graph g, int s, int t, int numV, int *path, int *length)
{
	int *isSelectMin = (int *)calloc(numV, sizeof(int));
	int *inPQ = (int *)calloc(numV, sizeof(int));
	double distance[numV], min_dist, w, total;
	int min, u;
	int previous[numV], tmp[numV];
	int n, output[numV], v;
	Dllist ptr, queue, node;

	for (int i = 0; i < numV; i++)
		distance[i] = INFINITIVE_VALUE;
	distance[s] = 0;
	previous[s] = s;

	queue = new_dllist();
	dll_append(queue, new_jval_i(s));

	while (!dll_empty(queue))
	{
		// get u from the priority queue
		min_dist = INFINITIVE_VALUE;
		dll_traverse(ptr, queue)
		{
			u = jval_i(ptr->val);
			if (min_dist > distance[u])
			{
				min_dist = distance[u];
				min = u;
				node = ptr;
			}
		}
		dll_delete_node(node);
		u = min;
		inPQ[u] = 0;

		if (u == t)
			break;

		isSelectMin[u] = 1;
		n = outdegree(g, u, output);
		for (int i = 0; i < n; i++)
		{
			v = output[i];
			if (!isSelectMin[v])
			{
				w = 1;
				if (distance[v] > distance[u] + w)
				{
					distance[v] = distance[u] + w;
					previous[v] = u;
				}
				if (!inPQ[v])
				{
					dll_append(queue, new_jval_i(v));
					inPQ[v] = 1;
				}
			}
		}
	}

	free(isSelectMin);
	free(inPQ);
	free(queue);

	total = distance[t];
	if (total != INFINITIVE_VALUE)
	{
		tmp[0] = t;
		n = 1;
		while (t != s)
		{
			t = previous[t];
			tmp[n++] = t;
		}
		for (int i = n - 1; i >= 0; i--)
			path[n - i - 1] = tmp[i];
		*length = n;
	}

	return total;
}

void dropGraph(Graph graph)
{
	JRB node, tree, tmp;
	jrb_traverse(node, graph.edges)
	{
		tree = (JRB)jval_v(node->val);
		jrb_traverse(tmp, tree)
		{
			jrb_free_tree(jval_v(tmp->val));
		}

		jrb_free_tree(tree);
	}

	jrb_free_tree(graph.edges);
	jrb_free_tree(graph.vertices);
	jrb_free_tree(graph.name2ID);
}
