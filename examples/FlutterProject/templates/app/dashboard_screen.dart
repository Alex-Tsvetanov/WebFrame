import 'dart:convert';
import 'package:flutter/material.dart';
import 'package:coroute_framework/coroute_framework.dart';

class DashboardScreen extends StatefulWidget {
  final Map<String, dynamic> model;

  const DashboardScreen({super.key, required this.model});

  @override
  State<DashboardScreen> createState() => _DashboardScreenState();
}

class _DashboardScreenState extends State<DashboardScreen> {
  late Map<String, dynamic> _model;
  bool _isLoading = false;

  @override
  void initState() {
    super.initState();
    _model = Map<String, dynamic>.from(widget.model);
  }

  List<Map<String, dynamic>> get _tasks {
    final raw = _model['tasks'];
    if (raw is List) return raw.cast<Map<String, dynamic>>();
    return const [];
  }

  Map<String, dynamic> get _stats {
    return (_model['stats'] as Map<String, dynamic>?) ?? const {};
  }

  bool get _authenticated => _model['authenticated'] as bool? ?? false;
  String get _username => _model['username'] as String? ?? '';

  Future<void> _refresh() async {
    setState(() => _isLoading = true);
    try {
      // Re-navigate to '/' to get a fresh model from the C++ backend
      if (mounted) {
        await Navigator.of(context).pushReplacement(
          MaterialPageRoute(builder: (_) => GenericView(route: '/')),
        );
      }
    } finally {
      if (mounted) setState(() => _isLoading = false);
    }
  }

  Future<void> _logout() async {
    try {
      await Bridge.submitAction('/api/logout', '{}');
      if (mounted) {
        await Navigator.of(context).pushReplacement(
          MaterialPageRoute(builder: (_) => GenericView(route: '/login')),
        );
      }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Logout failed: $e')),
        );
      }
    }
  }

  Future<void> _showCreateTaskDialog() async {
    final titleCtrl = TextEditingController();
    final descCtrl = TextEditingController();
    String? error;

    await showDialog<void>(
      context: context,
      builder: (ctx) => StatefulBuilder(
        builder: (ctx, setDialogState) => AlertDialog(
          title: const Text('New Task'),
          content: Column(
            mainAxisSize: MainAxisSize.min,
            crossAxisAlignment: CrossAxisAlignment.stretch,
            children: [
              if (error != null)
                Container(
                  margin: const EdgeInsets.only(bottom: 12),
                  padding: const EdgeInsets.all(10),
                  decoration: BoxDecoration(
                    color: const Color(0xFFFFF0F0),
                    borderRadius: BorderRadius.circular(6),
                  ),
                  child: Text(error!, style: const TextStyle(color: Color(0xFFC0392B), fontSize: 13)),
                ),
              TextField(
                controller: titleCtrl,
                decoration: const InputDecoration(labelText: 'Title *', hintText: 'Task title'),
                textInputAction: TextInputAction.next,
              ),
              const SizedBox(height: 12),
              TextField(
                controller: descCtrl,
                decoration: const InputDecoration(labelText: 'Description', hintText: 'Optional'),
                maxLines: 3,
              ),
            ],
          ),
          actions: [
            TextButton(onPressed: () => Navigator.pop(ctx), child: const Text('Cancel')),
            ElevatedButton(
              style: ElevatedButton.styleFrom(backgroundColor: const Color(0xFF6C63FF)),
              onPressed: () async {
                final title = titleCtrl.text.trim();
                if (title.isEmpty) {
                  setDialogState(() => error = 'Title is required');
                  return;
                }
                try {
                  final resp = await Bridge.submitAction(
                    '/api/tasks',
                    jsonEncode({'title': title, 'description': descCtrl.text.trim()}),
                  );
                  final data = jsonDecode(resp) as Map<String, dynamic>;
                  if (data.containsKey('id')) {
                    if (ctx.mounted) Navigator.pop(ctx);
                    await _refresh();
                  } else {
                    final msg = (data['error'] as Map?)?['message'] as String? ?? 'Failed to create';
                    setDialogState(() => error = msg);
                  }
                } catch (e) {
                  setDialogState(() => error = 'Error: $e');
                }
              },
              child: const Text('Create', style: TextStyle(color: Colors.white)),
            ),
          ],
        ),
      ),
    );
    titleCtrl.dispose();
    descCtrl.dispose();
  }

  Color _statusColor(String status) {
    switch (status) {
      case 'completed': return const Color(0xFF16A34A);
      case 'in_progress': return const Color(0xFF2563EB);
      default: return const Color(0xFFCA8A04);
    }
  }

  Color _statusBg(String status) {
    switch (status) {
      case 'completed': return const Color(0xFFDCFCE7);
      case 'in_progress': return const Color(0xFFDBEAFE);
      default: return const Color(0xFFFEF9C3);
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: const Color(0xFFF0F2F5),
      appBar: AppBar(
        backgroundColor: const Color(0xFF1A1A2E),
        title: const Text('Task Dashboard', style: TextStyle(color: Colors.white, fontSize: 18)),
        actions: [
          if (_authenticated) ...[
            Padding(
              padding: const EdgeInsets.symmetric(vertical: 16, horizontal: 8),
              child: Text(_username, style: const TextStyle(color: Color(0xFFCCCCCC), fontSize: 14)),
            ),
            TextButton(
              onPressed: _logout,
              child: const Text('Sign out', style: TextStyle(color: Color(0xFFAAAAAA))),
            ),
          ] else ...[
            TextButton(
              onPressed: () => Navigator.of(context).pushReplacement(
                MaterialPageRoute(builder: (_) => GenericView(route: '/login')),
              ),
              child: const Text('Sign in', style: TextStyle(color: Color(0xFFAAAAAA))),
            ),
          ],
          IconButton(
            icon: _isLoading
                ? const SizedBox(width: 20, height: 20, child: CircularProgressIndicator(color: Colors.white, strokeWidth: 2))
                : const Icon(Icons.refresh, color: Colors.white),
            onPressed: _isLoading ? null : _refresh,
          ),
        ],
      ),
      floatingActionButton: _authenticated
          ? FloatingActionButton(
              backgroundColor: const Color(0xFF6C63FF),
              onPressed: _showCreateTaskDialog,
              child: const Icon(Icons.add, color: Colors.white),
            )
          : null,
      body: RefreshIndicator(
        onRefresh: _refresh,
        child: CustomScrollView(
          slivers: [
            // Stats row
            SliverToBoxAdapter(
              child: Padding(
                padding: const EdgeInsets.fromLTRB(16, 16, 16, 8),
                child: GridView.count(
                  crossAxisCount: 4,
                  shrinkWrap: true,
                  physics: const NeverScrollableScrollPhysics(),
                  mainAxisSpacing: 8,
                  crossAxisSpacing: 8,
                  childAspectRatio: 1.6,
                  children: [
                    _StatCard(label: 'Total', value: '${_stats['total'] ?? 0}', color: const Color(0xFF6C63FF)),
                    _StatCard(label: 'Pending', value: '${_stats['pending'] ?? 0}', color: const Color(0xFFCA8A04)),
                    _StatCard(label: 'In Progress', value: '${_stats['in_progress'] ?? 0}', color: const Color(0xFF2563EB)),
                    _StatCard(label: 'Completed', value: '${_stats['completed'] ?? 0}', color: const Color(0xFF16A34A)),
                  ],
                ),
              ),
            ),
            SliverToBoxAdapter(
              child: Padding(
                padding: const EdgeInsets.fromLTRB(16, 8, 16, 8),
                child: Text('Tasks', style: Theme.of(context).textTheme.titleMedium?.copyWith(fontWeight: FontWeight.w700, color: const Color(0xFF1A1A2E))),
              ),
            ),
            if (_tasks.isEmpty)
              const SliverFillRemaining(
                child: Center(child: Text('No tasks yet. Tap + to create your first task!', style: TextStyle(color: Color(0xFFAAAAAA)))),
              )
            else
              SliverPadding(
                padding: const EdgeInsets.fromLTRB(16, 0, 16, 88),
                sliver: SliverList(
                  delegate: SliverChildBuilderDelegate(
                    (context, index) {
                      final task = _tasks[index];
                      final status = task['status'] as String? ?? 'pending';
                      return Card(
                        margin: const EdgeInsets.only(bottom: 8),
                        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(10)),
                        elevation: 1,
                        child: ListTile(
                          contentPadding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
                          title: Text(task['title'] as String? ?? '', style: const TextStyle(fontWeight: FontWeight.w600, color: Color(0xFF1A1A2E))),
                          subtitle: (task['description'] as String? ?? '').isNotEmpty
                              ? Text(task['description'] as String, style: const TextStyle(color: Color(0xFF888888), fontSize: 13))
                              : null,
                          trailing: Row(
                            mainAxisSize: MainAxisSize.min,
                            children: [
                              Container(
                                padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 4),
                                decoration: BoxDecoration(
                                  color: _statusBg(status),
                                  borderRadius: BorderRadius.circular(20),
                                ),
                                child: Text(status, style: TextStyle(color: _statusColor(status), fontSize: 12, fontWeight: FontWeight.w600)),
                              ),
                              const Icon(Icons.chevron_right, color: Color(0xFFAAAAAA)),
                            ],
                          ),
                          onTap: () => Navigator.of(context).push(
                            MaterialPageRoute(
                              builder: (_) => GenericView(route: '/task/${task['id']}'),
                            ),
                          ),
                        ),
                      );
                    },
                    childCount: _tasks.length,
                  ),
                ),
              ),
          ],
        ),
      ),
    );
  }
}

class _StatCard extends StatelessWidget {
  final String label;
  final String value;
  final Color color;

  const _StatCard({required this.label, required this.value, required this.color});

  @override
  Widget build(BuildContext context) {
    return Container(
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(10),
        boxShadow: const [BoxShadow(color: Color(0x0A000000), blurRadius: 8, offset: Offset(0, 2))],
      ),
      padding: const EdgeInsets.all(12),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Text(value, style: TextStyle(fontSize: 22, fontWeight: FontWeight.w700, color: color)),
          const SizedBox(height: 2),
          Text(label, style: const TextStyle(fontSize: 12, color: Color(0xFF888888))),
        ],
      ),
    );
  }
}
