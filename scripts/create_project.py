# Copyright (c) 2024 RIT Launch Initiative
# SPDX-License-Identifier: Apache-2.0

"""create_project.py

West extension command for creating new Zephyr projects from a template."""

import os
import shutil
import argparse
from pathlib import Path
from west.commands import WestCommand
from west import log


class CreateProjectCommand(WestCommand):
    """West command to create a new Zephyr project from template."""

    def __init__(self):
        super().__init__(
            'create-project',
            'create a new Zephyr project from template',
            '''\
Create a new Zephyr project from the template project.

This command copies the .template-project directory and customizes it
with the specified project name. The project will be created in the
specified location (e.g., app/samples or app/backplane).

Example usage:
  west create-project my_project app/samples
  west create-project sensor_module app/backplane
''')

    def do_add_parser(self, parser_adder):
        parser = parser_adder.add_parser(
            self.name,
            help=self.help,
            description=self.description,
            formatter_class=argparse.RawDescriptionHelpFormatter)

        parser.add_argument(
            'name',
            help='Name of the new project (e.g., my_project)')
        
        parser.add_argument(
            'location',
            help='Location where the project should be created (e.g., app/samples, app/backplane)')

        return parser

    def do_run(self, args, unknown_args):
        """Execute the create-project command."""
        project_name = args.name
        location = args.location

        # Get the FSW root directory (workspace root)
        fsw_root = self.topdir
        
        # Define template and target paths
        template_dir = Path(fsw_root) / 'app' / 'samples' / '.template-project'
        target_dir = Path(fsw_root) / location / project_name

        # Validate template exists
        if not template_dir.exists():
            log.err(f'Template directory not found: {template_dir}')
            return 1

        # Check if target directory already exists
        if target_dir.exists():
            log.err(f'Project directory already exists: {target_dir}')
            return 1

        # Create parent directory if it doesn't exist
        target_dir.parent.mkdir(parents=True, exist_ok=True)

        # Copy template to target location
        log.inf(f'Creating project "{project_name}" in {location}/')
        try:
            shutil.copytree(template_dir, target_dir)
        except Exception as e:
            log.err(f'Failed to copy template: {e}')
            return 1

        # Replace template placeholders in files
        try:
            self._customize_project(target_dir, project_name)
        except Exception as e:
            log.err(f'Failed to customize project: {e}')
            # Clean up partially created project
            shutil.rmtree(target_dir)
            return 1

        log.inf(f'Successfully created project at {target_dir}')
        log.inf(f'\nTo build your project, run:')
        log.inf(f'  west build -b <board> {location}/{project_name}')
        
        return 0

    def _customize_project(self, project_dir, project_name):
        """Replace template placeholders with actual project name."""
        # Convert project name to different formats
        # e.g., "my_project" -> "my-project", "my_project"
        project_name_underscore = project_name.replace('-', '_')
        project_name_hyphen = project_name.replace('_', '-')

        # Files that need to be updated
        files_to_update = [
            project_dir / 'CMakeLists.txt',
            project_dir / 'sample.yaml',
        ]

        for file_path in files_to_update:
            if not file_path.exists():
                continue
                
            # Read file content
            with open(file_path, 'r') as f:
                content = f.read()

            # Replace template placeholders
            # In CMakeLists.txt: project(template ...) -> project(project-name ...)
            content = content.replace('project(template', f'project({project_name_hyphen}')
            # In sample.yaml: name: template -> name: project-name
            content = content.replace('name: template', f'name: {project_name_hyphen}')
            # In sample.yaml: samples.template.default -> samples.project_name.default
            content = content.replace('samples.template.default', f'samples.{project_name_underscore}.default')

            # Write updated content back
            with open(file_path, 'w') as f:
                f.write(content)

        log.dbg(f'Customized project files with name: {project_name}')
