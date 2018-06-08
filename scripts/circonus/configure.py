#!/usr/bin/env python
#
# This script configures your Circonus account to collect goesrecv
# metrics, display them in a number of graphs (defined below), and
# present them on a dashboard.
#
# All managed graphs that are created through this script have a tag
# with the prefix 'goestools'. WARNING: graphs with this prefix that
# are NOT managed by this script will be deleted.
#
# Create an API token for this script by going to your Circonus
# account and navigating to 'Integrations' and 'API Tokens'.
#
# For instructions on how to use Circonus for goesrecv metrics, see
# https://pietern.github.io/goestools/guides/circonus.html.
#

import argparse
import json
import os
import requests

class Circonus:
  def __init__(self, token):
    self.token = token

  def headers(self):
    return {
      'Accept': 'application/json',
      'Content-Type': 'application/json',
      'X-Circonus-Auth-Token': self.token,
      'X-Circonus-App-Name': 'goestools',
    }

  def get(self, path, **kwargs):
    response = requests.get(
      "https://api.circonus.com/v2" + path,
      headers=self.headers(),
      **kwargs)
    response.raise_for_status()
    return response.json()

  def post(self, path, body, **kwargs):
    response = requests.post(
      "https://api.circonus.com/v2" + path,
      json.dumps(body),
      headers=self.headers(),
      **kwargs)
    response.raise_for_status()
    return response.json()

  def put(self, path, body, **kwargs):
    response = requests.put(
      "https://api.circonus.com/v2" + path,
      json.dumps(body),
      headers=self.headers(),
      **kwargs)
    #response.raise_for_status()
    return response.json()

  def delete(self, path, **kwargs):
    response = requests.delete(
      "https://api.circonus.com/v2" + path,
      headers=self.headers(),
      **kwargs)
    response.raise_for_status()
    return {}

  def get_check_bundle_for_host(self, host):
    bundles = self.get(
      '/check_bundle',
      params={'search': '(host:{})'.format(host)},
    )
    assert(len(bundles) == 1)
    return self.get(bundles[0]['_cid'], params={'query_broker': 1})

  def put_check_bundle_metrics(self, bundle, metrics):
    return self.put(
      '/check_bundle_metrics/{}'.format(bundle['_cid'].split('/').pop()),
      {'metrics': metrics},
    )

  def get_graphs_for_check(self, check_id):
    graphs = self.get(
      '/graph',
      params={
        'search': '(check_id:{})(tags:goestools:*)'.format(check_id),
      },
    )
    return graphs

class GraphBuilder:
  GRAPHS = {
    'viterbi': {
      'datapoints': [
        {
          'axis': 'l',
          'color': '#0000ff',
          'derive': 'gauge',
          'metric_name': 'statsd`viterbi_errors',
          'metric_type': 'histogram',
        },
      ],
      'max_left_y': 500,
      'title': '{prefix} Viterbi errors',
    },
    'viterbi_and_drops': {
      'datapoints': [
        {
          'axis': 'l',
          'color': '#0000ff',
          'derive': 'gauge',
          'metric_name': 'statsd`viterbi_errors',
          'metric_type': 'histogram',
        },
        {
          'axis': 'r',
          'color': '#ff0000',
          'derive': 'gauge',
          'metric_name': 'statsd`packets_dropped',
          'metric_type': 'numeric',
        },
      ],
      'max_left_y': 500,
      'min_right_y': 0,
      'title': '{prefix} Viterbi errors and drops',
    },
    'reed_solomon': {
      'datapoints': [
        {
          'axis': 'l',
          'color': '#ff0000',
          'derive': 'gauge',
          'metric_name': 'statsd`reed_solomon_errors',
          'metric_type': 'histogram',
        },
      ],
      'max_left_y': 75,
      'title': '{prefix} Reed-Solomon errors',
    },
    'demodulator': {
      'datapoints': [
        {
          'axis': 'l',
          'color': '#0000ff',
          'derive': 'gauge',
          'metric_name': 'statsd`frequency',
          'metric_type': 'numeric',
        },
        {
          'axis': 'r',
          'color': '#0000ff',
          'derive': 'gauge',
          'metric_name': 'statsd`omega',
          'metric_type': 'numeric',
        },
      ],
      'min_left_y': -10000,
      'max_left_y': +10000,
      'style': 'line',
      'title': '{prefix} demodulator',
    },
  }

  def __init__(self, bundle, check_id, prefix):
    self.bundle = bundle
    self.check_id = check_id
    self.prefix = prefix

  def build(self, name):
    datapoint_defaults = {
      'check_id': self.check_id,
      'data_formula': None,
      'legend_formula': None,
      'hidden': False,
      'stack': None,
    }

    v = self.GRAPHS[name]
    for datapoint in v['datapoints']:
      metric_name = datapoint['metric_name']
      for key, value in datapoint_defaults.items():
        datapoint.setdefault(key, value)
      datapoint.setdefault(
        'name', self.bundle['display_name'] + ': ' + metric_name)

    v['tags'] = v.get('tags', []) + ['goestools:' + name]
    v['title'] = v['title'].format(prefix=self.prefix)
    return v


def tags_to_dict(obj):
  return dict([tag.split(':', 2) for tag in obj['tags']])


def main():
  api_token_env_var = 'CIRCONUS_API_TOKEN'
  parser = argparse.ArgumentParser()
  parser.add_argument(
    '--api-token',
    type=str,
    metavar='TOKEN',
    help=(
     'If not specified, the contents of the environment variable '
      '{} is used'.format(api_token_env_var)
    ),
    default=os.getenv(api_token_env_var),
  )
  parser.add_argument(
    '--target',
    type=str,
    required=True,
    help='Host name associated with check to configure',
  )
  parser.add_argument(
    '--prefix',
    type=str,
    required=True,
    help='Prefix for graph titles'
  )

  args = parser.parse_args()

  metrics_numeric = [
    'statsd`frequency',
    'statsd`gain',
    'statsd`omega',
    'statsd`packets_dropped',
    'statsd`packets_ok',
  ]

  metrics_histogram = [
    'statsd`reed_solomon_errors',
    'statsd`viterbi_errors',
  ]

  c = Circonus(args.api_token)

  # Find check bundle that matches the target
  bundle = c.get_check_bundle_for_host(args.target)

  # There should be only 1 check for this bundle
  assert(len(bundle['_checks']))
  check = bundle['_checks'][0]
  check_id = check.split('/')[-1]
  print("Using check with ID: %s" % check_id)

  # Find metrics that are not yet active but should be
  metrics_activate = []
  for metric in bundle['metrics']:
    if metric['status'] == 'active':
      pass
    if metric['name'] in metrics_numeric:
      metric['status'] = 'active'
      metric['type'] = 'numeric'
      metrics_activate.append(metric)
    if metric['name'] in metrics_histogram:
      metric['status'] = 'active'
      metric['type'] = 'histogram'
      metrics_activate.append(metric)

  # Activate metrics that were not yet active
  if len(metrics_activate) > 0:
    c.put_check_bundle_metrics(bundle, metrics_activate)

  # Update set of goestools graphs for this check
  graph_definition_builder = GraphBuilder(bundle, check_id, args.prefix)
  graphs = c.get_graphs_for_check(check_id)
  graphs_to_create = set(GraphBuilder.GRAPHS.keys())
  graphs_to_update = {}
  graphs_to_delete = {}
  for graph in graphs:
    graph_id = tags_to_dict(graph).get('goestools', '')
    if graph_id in GraphBuilder.GRAPHS:
      graphs_to_update[graph_id] = graph
      graphs_to_create.remove(graph_id)
    else:
      graphs_to_delete[graph_id] = graph

  # Create
  for graph_id in graphs_to_create:
    print("Create graph: %s" % graph_id)
    c.post('/graph', graph_definition_builder.build(graph_id))

  # Update
  for graph_id, graph in graphs_to_update.items():
    print("Update graph: %s" % graph_id)
    c.put(graph['_cid'], graph_definition_builder.build(graph_id))

  # Delete
  for graph_id, graph in graphs_to_delete.items():
    print("Delete graph: %s" % graph_id)
    c.delete(graph['_cid'])

if __name__ == '__main__':
  main()
