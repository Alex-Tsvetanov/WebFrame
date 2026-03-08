import 'dart:convert';
import 'package:flutter/material.dart';
import 'package:coroute_framework/coroute_framework.dart';

class TaskDetailScreen extends StatefulWidget {
  final Map<String, dynamic> model;

  const TaskDetailScreen({super.key, required this.model});

  @override
  State<TaskDetailScreen> createState() => _TaskDetailScreenState();
}

class _TaskDetailScreenState extends State<TaskDetailScreen> {
  late String _status;
  String? _statusMessage;
  bool _isSuccess = false;
  bool _isSaving = false;

  @override
  void initState() {
    super.initState();
    _status = widget.model['status'] as String? ?? 'pending';
  }

  int get _taskId => (widget.model['id'] as num?)?.toInt() ?? 0;
  String get _title => widget.model['title'] as String? ?? '';
  String get _description => widget.model['description'] as String? ?? '';

  Future<void> _updateStatus() async {
    setState(() {
      _isSaving = true;
      _statusMessage = null;
    });
    try {
      final resp = await Bridge.submitAction(
        '/api/tasks/$_taskId',
        jsonEncode({'status': _status}),
      );
      final data = jsonDecode(resp) as Map<String, dynamic>;
      if (data.containsKey('id')) {
        setState(() {
          _isSuccess = true;
          _statusMessage = 'Status updated successfully';
        });
      } else {
        final msg = (data['error'] as Map?)?['message'] as String? ?? 'Update failed';
        setState(() {
          _isSuccess = false;
          _statusMessage = msg;
        });
      }
    } catch (e) {
      setState(() {
        _isSuccess = false;
        _statusMessage = 'Error: $e';
      });
    } finally {
      if (mounted) setState(() => _isSaving = false);
    }
  }

  Future<void> _deleteTask() async {
    final confirmed = await showDialog<bool>(
      context: context,
      builder: (ctx) => AlertDialog(
        title: const Text('Delete Task'),
        content: const Text('This cannot be undone. Are you sure?'),
        actions: [
          TextButton(onPressed: () => Navigator.pop(ctx, false), child: const Text('Cancel')),
          ElevatedButton(
            style: ElevatedButton.styleFrom(backgroundColor: const Color(0xFFE74C3C)),
            onPressed: () => Navigator.pop(ctx, true),
            child: const Text('Delete', style: TextStyle(color: Colors.white)),
          ),
        ],
      ),
    );

    if (confirmed != true || !mounted) return;

    try {
      // Bridge.submitAction maps to PUT/POST; for DELETE we pass a sentinel body
      await Bridge.submitAction('/api/tasks/$_taskId/delete', '{}');
      if (mounted) {
        await Navigator.of(context).pushReplacement(
          MaterialPageRoute(builder: (_) => GenericView(route: '/')),
        );
      }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context)
            .showSnackBar(SnackBar(content: Text('Delete failed: $e')));
      }
    }
  }

  Color _statusColor(String status) {
    switch (status) {
      case 'completed':  return const Color(0xFF16A34A);
      case 'in_progress': return const Color(0xFF2563EB);
      default: return const Color(0xFFCA8A04);
    }
  }

  Color _statusBg(String status) {
    switch (status) {
      case 'completed':  return const Color(0xFFDCFCE7);
      case 'in_progress': return const Color(0xFFDBEAFE);
      default: return const Color(0xFFFEF9C3);
    }
  }

  @override
  Widget build(BuildContext context) {
    if (_taskId == 0) {
      return Scaffold(
        appBar: AppBar(
          backgroundColor: const Color(0xFF1A1A2E),
          title: const Text('Task Dashboard', style: TextStyle(color: Colors.white)),
          leading: BackButton(
            color: Colors.white,
            onPressed: () => Navigator.of(context).pushReplacement(
              MaterialPageRoute(builder: (_) => GenericView(route: '/')),
            ),
          ),
        ),
        body: const Center(child: Text('Task not found', style: TextStyle(color: Color(0xFF888888)))),
      );
    }

    return Scaffold(
      backgroundColor: const Color(0xFFF0F2F5),
      appBar: AppBar(
        backgroundColor: const Color(0xFF1A1A2E),
        title: const Text('Task Details', style: TextStyle(color: Colors.white)),
        leading: BackButton(
          color: Colors.white,
          onPressed: () => Navigator.of(context).maybePop().then((popped) {
            if (!popped && context.mounted) {
              Navigator.of(context).pushReplacement(
                MaterialPageRoute(builder: (_) => GenericView(route: '/')),
              );
            }
          }),
        ),
        actions: [
          IconButton(
            icon: const Icon(Icons.delete_outline, color: Color(0xFFE74C3C)),
            onPressed: _deleteTask,
            tooltip: 'Delete task',
          ),
        ],
      ),
      body: SingleChildScrollView(
        padding: const EdgeInsets.all(16),
        child: Card(
          shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
          elevation: 1,
          child: Padding(
            padding: const EdgeInsets.all(24),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  _title,
                  style: const TextStyle(fontSize: 22, fontWeight: FontWeight.w700, color: Color(0xFF1A1A2E)),
                ),
                const SizedBox(height: 4),
                Text('ID #$_taskId', style: const TextStyle(color: Color(0xFF888888), fontSize: 13)),
                const SizedBox(height: 16),
                if (_description.isNotEmpty) ...[
                  Text(_description, style: const TextStyle(fontSize: 15, color: Color(0xFF444444), height: 1.5)),
                  const SizedBox(height: 16),
                ],
                // Current status badge
                Row(
                  children: [
                    Container(
                      padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 5),
                      decoration: BoxDecoration(
                        color: _statusBg(_status),
                        borderRadius: BorderRadius.circular(20),
                      ),
                      child: Text(
                        _status,
                        style: TextStyle(color: _statusColor(_status), fontSize: 13, fontWeight: FontWeight.w600),
                      ),
                    ),
                  ],
                ),
                const SizedBox(height: 24),
                const Divider(),
                const SizedBox(height: 16),
                const Text(
                  'Change Status',
                  style: TextStyle(fontSize: 14, fontWeight: FontWeight.w600, color: Color(0xFF444444)),
                ),
                const SizedBox(height: 8),
                DropdownButtonFormField<String>(
                  value: _status,
                  decoration: InputDecoration(
                    border: OutlineInputBorder(borderRadius: BorderRadius.circular(8), borderSide: const BorderSide(color: Color(0xFFDDDDDD))),
                    contentPadding: const EdgeInsets.symmetric(horizontal: 14, vertical: 10),
                  ),
                  items: const [
                    DropdownMenuItem(value: 'pending', child: Text('Pending')),
                    DropdownMenuItem(value: 'in_progress', child: Text('In Progress')),
                    DropdownMenuItem(value: 'completed', child: Text('Completed')),
                  ],
                  onChanged: (v) { if (v != null) setState(() => _status = v); },
                ),
                const SizedBox(height: 12),
                if (_statusMessage != null)
                  Container(
                    margin: const EdgeInsets.only(bottom: 12),
                    padding: const EdgeInsets.all(10),
                    decoration: BoxDecoration(
                      color: _isSuccess ? const Color(0xFFDCFCE7) : const Color(0xFFFFF0F0),
                      borderRadius: BorderRadius.circular(8),
                    ),
                    child: Text(
                      _statusMessage!,
                      style: TextStyle(
                        color: _isSuccess ? const Color(0xFF166534) : const Color(0xFFC0392B),
                        fontSize: 13,
                      ),
                    ),
                  ),
                ElevatedButton(
                  onPressed: _isSaving ? null : _updateStatus,
                  style: ElevatedButton.styleFrom(
                    backgroundColor: const Color(0xFF6C63FF),
                    minimumSize: const Size(double.infinity, 44),
                    shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(8)),
                  ),
                  child: _isSaving
                      ? const SizedBox(width: 20, height: 20, child: CircularProgressIndicator(color: Colors.white, strokeWidth: 2))
                      : const Text('Save Changes', style: TextStyle(fontSize: 15, fontWeight: FontWeight.w600, color: Colors.white)),
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}
