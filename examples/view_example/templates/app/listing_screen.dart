import 'package:flutter/material.dart';
import 'package:coroute_framework/generic_view.dart';

class ListingScreen extends StatelessWidget {
  final Map<String, dynamic> model;

  const ListingScreen({
    super.key,
    required this.model,
  });

  @override
  Widget build(BuildContext context) {
    final title = model['title'] ?? 'Listing';
    final items = List<String>.from(model['items'] ?? []);

    return Scaffold(
      body: CustomScrollView(
        slivers: [
          SliverAppBar(
            title: Text(title),
            floating: true,
            actions: [
              IconButton(
                icon: const Icon(Icons.person),
                onPressed: () => Navigator.push(
                  context,
                  MaterialPageRoute(builder: (_) => const GenericView(route: '/profile')),
                ),
              ),
            ],
          ),
          SliverList(
            delegate: SliverChildBuilderDelegate(
              (context, index) {
                final item = items[index];
                return ListTile(
                  leading: CircleAvatar(child: Text(item[0])),
                  title: Text(item),
                  trailing: const Icon(Icons.chevron_right),
                  onTap: () => Navigator.push(
                    context,
                    MaterialPageRoute(builder: (_) => GenericView(route: '/user/$item')),
                  ),
                );
              },
              childCount: items.length,
            ),
          ),
        ],
      ),
    );
  }
}
