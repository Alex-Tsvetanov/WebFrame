// Task Dashboard - Client-side JavaScript

class TaskDashboard {
    constructor() {
        this.tasks = [];
        this.ws = null;
        this.reconnectAttempts = 0;
        this.maxReconnectAttempts = 5;

        this.init();
    }

    init() {
        // Create toast container
        this.createToastContainer();

        // Load initial data
        this.loadTasks();
        this.loadStats();

        // Connect WebSocket
        this.connectWebSocket();

        // Setup event listeners
        this.setupEventListeners();
    }

    // Toast notification system
    createToastContainer() {
        if (!document.getElementById('toast-container')) {
            const container = document.createElement('div');
            container.id = 'toast-container';
            container.style.cssText = 'position: fixed; top: 20px; right: 20px; z-index: 9999; display: flex; flex-direction: column; gap: 10px;';
            document.body.appendChild(container);
        }
    }

    showToast(message, type = 'info') {
        console.log('showToast called:', message, type);

        // Ensure container exists
        this.createToastContainer();

        const container = document.getElementById('toast-container');
        if (!container) {
            console.error('Toast container not found!');
            alert(message); // Fallback to alert
            return;
        }

        const toast = document.createElement('div');

        const colors = {
            success: '#10b981',
            error: '#ef4444',
            warning: '#f59e0b',
            info: '#3b82f6'
        };

        toast.style.cssText = `
            padding: 12px 20px;
            background: ${colors[type] || colors.info};
            color: white;
            border-radius: 8px;
            box-shadow: 0 4px 12px rgba(0,0,0,0.15);
            font-size: 14px;
            max-width: 350px;
            animation: slideIn 0.3s ease;
        `;
        toast.textContent = message;

        container.appendChild(toast);
        console.log('Toast added to container');

        // Auto-remove after 4 seconds
        setTimeout(() => {
            toast.style.animation = 'slideOut 0.3s ease';
            setTimeout(() => toast.remove(), 300);
        }, 4000);
    }

    // WebSocket connection
    connectWebSocket() {
        const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
        const wsUrl = `${protocol}//${window.location.host}/ws`;

        this.ws = new WebSocket(wsUrl);

        this.ws.onopen = () => {
            console.log('WebSocket connected');
            this.reconnectAttempts = 0;
            this.updateConnectionStatus('connected');

            // Ping to trigger server to send pending messages
            this.pingInterval = setInterval(() => {
                if (this.ws && this.ws.readyState === WebSocket.OPEN) {
                    this.ws.send(JSON.stringify({ type: 'ping' }));
                }
            }, 1000); // Ping every 1 second - balance between responsiveness and overhead
        };

        this.ws.onclose = () => {
            console.log('WebSocket disconnected');
            this.updateConnectionStatus('disconnected');
            if (this.pingInterval) {
                clearInterval(this.pingInterval);
                this.pingInterval = null;
            }
            this.scheduleReconnect();
        };

        this.ws.onerror = (error) => {
            console.error('WebSocket error:', error);
        };

        this.ws.onmessage = (event) => {
            try {
                const data = JSON.parse(event.data);
                this.handleWebSocketMessage(data);
            } catch (e) {
                console.error('Failed to parse WebSocket message:', e);
                console.error('Raw message:', event.data);
            }
        };
    }

    scheduleReconnect() {
        if (this.reconnectAttempts < this.maxReconnectAttempts) {
            this.reconnectAttempts++;
            const delay = Math.min(1000 * Math.pow(2, this.reconnectAttempts), 30000);
            console.log(`Reconnecting in ${delay}ms...`);
            setTimeout(() => this.connectWebSocket(), delay);
        }
    }

    updateConnectionStatus(status) {
        const el = document.getElementById('connection-status');
        if (el) {
            el.className = `connection-status ${status}`;
            el.querySelector('.status-text').textContent =
                status === 'connected' ? 'Connected' : 'Disconnected';
        }
    }

    handleWebSocketMessage(data) {
        console.log('WebSocket message:', data);

        switch (data.type) {
            case 'task_created':
                this.onTaskCreated(data.data);
                break;
            case 'task_updated':
                this.onTaskUpdated(data.data);
                break;
            case 'task_deleted':
                this.onTaskDeleted(data.data.id);
                break;
            case 'connected':
                console.log('Server says:', data.message);
                break;
        }
    }

    // API calls
    async loadTasks() {
        try {
            const response = await fetch('/api/tasks');
            this.tasks = await response.json();
            this.renderTasks();
        } catch (e) {
            console.error('Failed to load tasks:', e);
        }
    }

    async loadStats() {
        try {
            const response = await fetch('/api/stats/tasks');
            const stats = await response.json();
            this.renderStats(stats);
        } catch (e) {
            console.error('Failed to load stats:', e);
        }
    }

    async createTask(title, description) {
        console.log('createTask called');
        try {
            const response = await fetch('/api/tasks', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ title, description })
            });

            console.log('createTask response:', response.status, response.ok);

            if (!response.ok) {
                console.log('Response not ok, status:', response.status);

                // Handle auth errors immediately without parsing JSON
                if (response.status === 401) {
                    console.log('401 - calling showToast');
                    this.showToast('Please login to create tasks', 'warning');
                    setTimeout(() => window.location.href = '/login', 1500);
                    return false;
                }

                // Try to get error message for other errors
                let message = 'Failed to create task';
                try {
                    const error = await response.json();
                    message = error.error?.message || message;
                    console.log('Error message:', message);
                } catch (jsonErr) {
                    console.log('Could not parse error response as JSON');
                }

                this.showToast(message, 'error');
                return false;
            }

            // Don't reload - WebSocket will push the update
            return true;
        } catch (e) {
            console.error('createTask error:', e);
            this.showToast('Network error. Please try again.', 'error');
            return false;
        }
    }

    async updateTaskStatus(id, status) {
        try {
            const response = await fetch(`/api/tasks/${id}`, {
                method: 'PUT',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ status })
            });

            if (!response.ok) {
                let message = 'Failed to update task';
                try {
                    const error = await response.json();
                    message = error.error?.message || message;
                } catch (jsonErr) { }

                if (response.status === 401) {
                    this.showToast('Please login to update tasks', 'warning');
                    setTimeout(() => window.location.href = '/login', 1500);
                } else {
                    this.showToast(message, 'error');
                }
            }
        } catch (e) {
            console.error('updateTaskStatus error:', e);
            this.showToast('Failed to update task', 'error');
        }
    }

    async deleteTask(id) {
        if (!confirm('Delete this task?')) return;

        try {
            const response = await fetch(`/api/tasks/${id}`, { method: 'DELETE' });

            if (!response.ok) {
                let message = 'Failed to delete task';
                try {
                    const error = await response.json();
                    message = error.error?.message || message;
                } catch (jsonErr) { }

                if (response.status === 401) {
                    this.showToast('Please login to delete tasks', 'warning');
                    setTimeout(() => window.location.href = '/login', 1500);
                } else {
                    this.showToast(message, 'error');
                }
            }
        } catch (e) {
            console.error('deleteTask error:', e);
            this.showToast('Failed to delete task', 'error');
        }
    }

    // WebSocket event handlers
    onTaskCreated(task) {
        this.tasks.unshift(task);
        this.renderTasks();
        this.loadStats();

        // Highlight new task
        setTimeout(() => {
            const el = document.querySelector(`[data-task-id="${task.id}"]`);
            if (el) el.classList.add('new');
        }, 50);
    }

    onTaskUpdated(task) {
        const index = this.tasks.findIndex(t => t.id === task.id);
        if (index !== -1) {
            this.tasks[index] = task;
            this.renderTasks();
            this.loadStats();
        }
    }

    onTaskDeleted(id) {
        this.tasks = this.tasks.filter(t => t.id !== id);
        this.renderTasks();
        this.loadStats();
    }

    // Rendering
    renderTasks() {
        const container = document.getElementById('task-list');
        if (!container) return;

        if (this.tasks.length === 0) {
            container.innerHTML = '<p style="text-align: center; color: var(--gray-500);">No tasks yet. Create one!</p>';
            return;
        }

        container.innerHTML = this.tasks.map(task => this.renderTaskCard(task)).join('');
    }

    renderTaskCard(task) {
        const statusClass = task.status.replace('_', '-');
        return `
            <div class="task-card" data-task-id="${task.id}">
                <div class="task-status ${task.status}" 
                     onclick="dashboard.cycleStatus(${task.id}, '${task.status}')"
                     title="Click to change status"></div>
                <div class="task-content">
                    <div class="task-title">${this.escapeHtml(task.title)}</div>
                    ${task.description ? `<div class="task-description">${this.escapeHtml(task.description)}</div>` : ''}
                </div>
                <div class="task-actions">
                    <button class="btn btn-sm btn-outline" onclick="dashboard.deleteTask(${task.id})">Delete</button>
                </div>
            </div>
        `;
    }

    renderStats(stats) {
        document.getElementById('stat-total').textContent = stats.total;
        document.getElementById('stat-pending').textContent = stats.pending;
        document.getElementById('stat-progress').textContent = stats.in_progress;
        document.getElementById('stat-completed').textContent = stats.completed;
    }

    // Helpers
    cycleStatus(id, currentStatus) {
        const statusOrder = ['pending', 'in_progress', 'completed'];
        const currentIndex = statusOrder.indexOf(currentStatus);
        const nextStatus = statusOrder[(currentIndex + 1) % statusOrder.length];
        this.updateTaskStatus(id, nextStatus);
    }

    escapeHtml(text) {
        const div = document.createElement('div');
        div.textContent = text;
        return div.innerHTML;
    }

    // Event listeners
    setupEventListeners() {
        // Add task button
        const addBtn = document.getElementById('add-task-btn');
        if (addBtn) {
            addBtn.addEventListener('click', () => this.openModal());
        }

        // Task form
        const form = document.getElementById('task-form');
        if (form) {
            form.addEventListener('submit', async (e) => {
                e.preventDefault();
                const title = document.getElementById('task-title').value;
                const description = document.getElementById('task-description').value;

                if (await this.createTask(title, description)) {
                    this.closeModal();
                    form.reset();
                }
            });
        }

        // Close modal on backdrop click
        const modal = document.getElementById('task-modal');
        if (modal) {
            modal.addEventListener('click', (e) => {
                if (e.target === modal) this.closeModal();
            });
        }

        // Close modal on Escape
        document.addEventListener('keydown', (e) => {
            if (e.key === 'Escape') this.closeModal();
        });
    }

    openModal() {
        const modal = document.getElementById('task-modal');
        if (modal) {
            modal.classList.add('active');
            document.getElementById('task-title').focus();
        }
    }

    closeModal() {
        const modal = document.getElementById('task-modal');
        if (modal) {
            modal.classList.remove('active');
        }
    }
}

// Global instance
let dashboard;

// Initialize when DOM is ready
document.addEventListener('DOMContentLoaded', () => {
    dashboard = new TaskDashboard();
});

// Global functions for inline handlers
function closeModal() {
    dashboard?.closeModal();
}
