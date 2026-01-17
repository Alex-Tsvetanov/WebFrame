setTimeout(() => {
    throw new Error('I throw right after creation');
}, 0);
